#include "ConnectTask.h"
#include "LEDTask/LEDTask.h"
#include "PumpTask/PumpTask.h"


// Các hằng số cấu hình
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint16_t reconnectInterval = 180000U; // 3 phút connect module

// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
uint32_t previousReconnectCheck;


WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);


// Cấu hình WiFi
constexpr char WIFI_SSID[] = "E11_12";
constexpr char WIFI_PASSWORD[] = "Tiger@E1112";

// constexpr char WIFI_SSID[] = "GGROUP-LAU5";
// constexpr char WIFI_PASSWORD[] = "Ggr0up5#";


// Cấu hình ThingsBoard
constexpr char TOKEN[] = "14l4hG8NU4QXotadl8J4";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

// Mảng chứa callback RPC
const std::array<RPC_Callback, 4U> callbacks = {
    RPC_Callback{ "setValueButtonLED", setLedSwitchState },
    RPC_Callback{ "getValueButtonLED", getLedState },
    RPC_Callback{ "setValueButtonPump", setPumpState }, 
    RPC_Callback{ "getValueButtonPump", getPumpState }
};


// Task kết nối WiFi
void WiFiTask(void *pvParameters) {
    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Connecting to WiFi...");
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            while (WiFi.status() != WL_CONNECTED) {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                Serial.print(".");
            }
            Serial.println("Connected to WiFi");
        }
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

// Task kết nối ThingsBoard
void ThingsBoardTask(void *pvParameters) {
    for (;;) {
        // Kiểm tra kết nối ThingsBoard, nếu chưa kết nối thì thực hiện kết nối
        if (!tb.connected()) {
            Serial.println("Connecting to ThingsBoard...");
            if (tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
                Serial.println("Connected to ThingsBoard");
                // Gửi MAC address và đăng ký RPC
                tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
                tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend());

                // Gửi trạng thái hiện tại của máy bơm để dashboard cập nhật
                tb.sendAttributeData("ledState", ledState);
                tb.sendAttributeData("pumpState", pumpState);
            }
            else {
                Serial.println("Failed to connect");
            }
        }
        // Nếu có thay đổi thuộc tính, cập nhật lên ThingsBoard
        if (attributesChangedLED) {
            attributesChangedLED = false;
            tb.sendAttributeData("getValueButtonLED", digitalRead(LED_PIN));
        }
        if (attributesChangedPump) {
            attributesChangedPump = false;
            tb.sendAttributeData("getValueButtonPump", pumpState);
        }
        tb.loop();
        vTaskDelay(800 / portTICK_PERIOD_MS);
    }
}

// Task kiểm tra và kết nối lại WiFi và ThingsBoard
void ReconnectTask(void *pvParameters) {
    for (;;) {
        if (millis() - previousReconnectCheck >= reconnectInterval) {
            previousReconnectCheck = millis();
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("Reconnecting WiFi...");
                WiFi.disconnect();
                WiFi.reconnect();
            }
            if (!tb.connected()) {
                Serial.println("Reconnecting to ThingsBoard...");
                tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
            }
        }
        vTaskDelay(reconnectInterval / portTICK_PERIOD_MS);
    }
}