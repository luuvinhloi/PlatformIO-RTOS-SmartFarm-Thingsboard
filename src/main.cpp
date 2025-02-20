#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Arduino_MQTT_Client.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_Sensor.h>
#include <ThingsBoard.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// Địa chỉ I2C của màn hình LCD
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2


#define LED_PIN 15 // Chân kết nối LED
#define DHT_PIN 14  // Chân kết nối cảm biến nhiệt độ độ ẩm DHT11
#define SOIL_MOISTURE_PIN 34  // Chân kết nối cảm biến độ ẩm đất
#define RELAY_PIN 26       // Relay
#define BUTTON_PIN 16      // Button Pump
#define MODE_BUTTON_PIN 17 // Button Auto/Manual mode


// constexpr char WIFI_SSID[] = "E11_12";
// constexpr char WIFI_PASSWORD[] = "Tiger@E1112";

constexpr char WIFI_SSID[] = "GGROUP-LAU5";
constexpr char WIFI_PASSWORD[] = "Ggr0up5#";

constexpr char TOKEN[] = "snd4dtdskqneltfh76yk";


constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;


constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;
constexpr uint16_t reconnectInterval = 180000U; // 3 phút


constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_MODE_ATTR[] = "ledMode";
constexpr char LED_STATE_ATTR[] = "ledState";

volatile bool attributesChanged = false;
volatile int ledMode = 0;
volatile bool ledState = false;

bool isAutoMode = true;    // Auto Mode
bool pumpState = false;    // Trạng thái của Pump

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;

uint32_t previousStateChange;

constexpr int16_t telemetrySendInterval = 10000U;
uint32_t previousDataSend;
uint32_t previousReconnectCheck;

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
  LED_STATE_ATTR,
  BLINKING_INTERVAL_ATTR
};

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);

DHT dht(DHT_PIN, DHT11);

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// Toạ độ GPS của thiết bị
constexpr float latitude = 10.898576;
constexpr float longitude = 106.794645;


RPC_Response setLedSwitchState(const RPC_Data &data) {
  Serial.println("Received Switch state");
  bool newState = data;
  Serial.print("Bật công tắc đèn: ");
  Serial.println(newState);
  digitalWrite(LED_PIN, newState);
  attributesChanged = true;
  return RPC_Response("setValueButtonLED", newState);
}

const std::array<RPC_Callback, 1U> callbacks = {
  RPC_Callback{ "setValueButtonLED", setLedSwitchState }
};


void WiFiTask(void *pvParameters);
void ThingsBoardTask(void *pvParameters);
void SensorTask(void *pvParameters);
void ReconnectTask(void *pvParameters);
void TaskPumpControl(void *pvParameters);
void TaskButtonControl(void *pvParameters);
void TaskModeControl(void *pvParameters);


void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);

  pinMode(LED_PIN, OUTPUT);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  dht.begin();

  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();  // Bật đèn nền
  lcd.setCursor(0, 0);
  lcd.print("Smart Farm!");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  // Khởi tạo các task của FreeRTOS
  xTaskCreate(WiFiTask, "WiFiTask", 4096, NULL, 1, NULL);
  xTaskCreate(ThingsBoardTask, "ThingsBoardTask", 4096, NULL, 1, NULL);
  xTaskCreate(SensorTask, "SensorTask", 4096, NULL, 1, NULL);
  xTaskCreate(ReconnectTask, "ReconnectTask", 4096, NULL, 1, NULL);
  xTaskCreate(TaskPumpControl, "TaskPumpControl", 1000, NULL, 1, NULL);
  xTaskCreate(TaskButtonControl, "TaskButtonControl", 1000, NULL, 1, NULL);
  xTaskCreate(TaskModeControl, "TaskModeControl", 1000, NULL, 1, NULL);
}


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
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}


void ThingsBoardTask(void *pvParameters) {
  for (;;) {
    if (!tb.connected()) {
      Serial.println("Connecting to ThingsBoard...");
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
          Serial.println("Failed to connect");
      } else {
          Serial.println("Connected to ThingsBoard");
          tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
          tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend());
      }
    }
    if (attributesChanged) {
      attributesChanged = false;
      tb.sendAttributeData("ledState", digitalRead(LED_PIN));
    }
    tb.loop();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}


void SensorTask(void *pvParameters) {
  for (;;) {
    if (millis() - previousDataSend > telemetrySendInterval) {
      previousDataSend = millis();
      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();
      int soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);
      float soilMoisturePercent = map(soilMoistureRaw, 4095, 0, 0, 100);
      
      Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%, Soil Moisture: %.2f%%\n", temperature, humidity, soilMoisturePercent);
      
      // Hiển thị dữ liệu lên LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.printf("T:%.1fC H:%.1f%%", temperature, humidity);
      lcd.setCursor(0, 1);
      lcd.printf("Soil: %.1f%%", soilMoisturePercent);
      
      // Gửi dữ liệu lên ThingsBoard
      if (!isnan(temperature) && !isnan(humidity)) {
        tb.sendTelemetryData("temperature", temperature);
        tb.sendTelemetryData("humidity", humidity);
      }
      tb.sendTelemetryData("soil_moisture", soilMoisturePercent);

      // Gửi dữ liệu GPS lên ThingsBoard
      tb.sendTelemetryData("lat", latitude);
      tb.sendTelemetryData("long", longitude);

      Serial.print("Sent GPS Data: Latitude: ");
      Serial.print(latitude);
      Serial.print(", Longitude: ");
      Serial.println(longitude);
    }
    vTaskDelay(telemetrySendInterval / portTICK_PERIOD_MS);
  }
}


void ReconnectTask(void *pvParameters) {
  for (;;) {
    if (millis() - previousReconnectCheck > reconnectInterval) {
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


void turnOnOffPump(int onOff) {
  digitalWrite(RELAY_PIN, onOff);
}

// Task điều khiển Pump auto mode
void TaskPumpControl(void *pvParameters) {
  while(1) {
    if (isAutoMode) { // Chỉ chạy khi đang ở Auto mode
      turnOnOffPump(1); // Bật Pump
      Serial.println("Máy bơm (Auto): Đang bật!");
      vTaskDelay(10000 / portTICK_PERIOD_MS); // Chờ 10s
      
      turnOnOffPump(0); // Tắt Pump
      Serial.println("Máy bơm (Auto): Đang tắt!");
      vTaskDelay(3000 / portTICK_PERIOD_MS); // Chờ 3s
    } else {
      vTaskDelay(500 / portTICK_PERIOD_MS); // Nếu ở Manual mode thì chờ ngắn
    }
  }
}

// Task điều khiển Pump bằng Button ở Manual mode
void TaskButtonControl(void *pvParameters) {
  bool lastButtonState = HIGH; // Lưu trạng thái trước của nút nhấn
  while(1) {
    if (!isAutoMode) { // Chỉ chạy khi đang ở Manual Mode
      bool currentButtonState = digitalRead(BUTTON_PIN);
      
      if (currentButtonState == LOW && lastButtonState == HIGH) { // Khi nút được nhấn
        pumpState = !pumpState; // Đảo trạng thái máy bơm
        turnOnOffPump(pumpState); // Bật hoặc tắt máy bơm
        Serial.print("Máy bơm (Manual): ");
        Serial.println(pumpState ? "Đang bật!" : "Đang tắt!");
      }

      lastButtonState = currentButtonState;
    }
    vTaskDelay(200 / portTICK_PERIOD_MS); // Debounce
  }
}

// Task xử lý Button chuyển đổi Auto/Manual mode
void TaskModeControl(void *pvParameters) {
  bool lastState = HIGH; // Trạng thái trước của button mode
  while(1) {
    bool currentState = digitalRead(MODE_BUTTON_PIN);
    if (currentState == LOW && lastState == HIGH) { // Button mode được nhấn
      isAutoMode = !isAutoMode; // Chuyển đổi mode
      Serial.print("Chế độ: ");
      Serial.println(isAutoMode ? "Auto" : "Manual");
    }
    lastState = currentState;
    vTaskDelay(200 / portTICK_PERIOD_MS); // Tránh lặp lại nhanh
  }
}


void loop() {
  vTaskDelete(NULL);
}