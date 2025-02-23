#include <ArduinoOTA.h>
#include <Arduino_MQTT_Client.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_Sensor.h>
#include <ThingsBoard.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <array>


#define TELEGRAM_BOT_TOKEN "8122936180:AAHVsRVDZrjwevWBwJnzZOHm_dgcJgkevuY"
#define TELEGRAM_CHAT_ID "1378242143"


// Địa chỉ I2C của màn hình LCD
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2


// Định nghĩa chân kết nối
#define LED_PIN 15 // Chân kết nối LED
#define LED_BUTTON_PIN 33 // Button LED
#define DHT_PIN 14  // Chân kết nối cảm biến nhiệt độ độ ẩm DHT11
#define SOIL_MOISTURE_PIN 34  // Chân kết nối cảm biến độ ẩm đất
#define RELAY_PIN 26       // Relay
#define BUTTON_PIN 16      // Button Pump
#define MODE_BUTTON_PIN 17 // Button Auto/Manual mode


// Cấu hình WiFi
constexpr char WIFI_SSID[] = "E11_12";
constexpr char WIFI_PASSWORD[] = "Tiger@E1112";

// constexpr char WIFI_SSID[] = "GGROUP-LAU5";
// constexpr char WIFI_PASSWORD[] = "Ggr0up5#";


// Cấu hình ThingsBoard
constexpr char TOKEN[] = "snd4dtdskqneltfh76yk";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;


// Các hằng số cấu hình
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;
constexpr uint16_t reconnectInterval = 180000U; // 3 phút
constexpr int16_t telemetrySendInterval = 10000U; // 10 giây
constexpr uint32_t alertInterval = 60000; // 10 phút (600.000 milliseconds)


// Các thuộc tính ThingsBoard
constexpr char LED_STATE_ATTR[] = "ledState";


// Các biến trạng thái
volatile bool attributesChanged = false;
volatile bool ledState = false;
bool isAutoMode = true;    // Auto Mode
bool pumpState = false;    // Trạng thái của Pump


// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
uint32_t previousDataSend;
uint32_t previousReconnectCheck;
uint32_t lastAlertTime = 0;

constexpr std::array<const char *, 1U> SHARED_ATTRIBUTES_LIST = {
  LED_STATE_ATTR
};


// Khởi tạo các đối tượng
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);
DHT dht(DHT_PIN, DHT11);
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);


// Toạ độ GPS của thiết bị
constexpr float latitude = 10.898576;
constexpr float longitude = 106.794645;


// Gửi tin nhắn cảnh báo qua Telegram
void sendTelegramMessage(String message, String imageUrl) {
  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + 
              "/sendPhoto?chat_id=" + String(TELEGRAM_CHAT_ID) + 
              "&photo=" + imageUrl + 
              "&caption=" + message;

  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.println("Gửi tin nhắn Telegram thành công!");
  } else {
    Serial.print("Lỗi gửi tin nhắn Telegram");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void checkAndSendAlerts(float humidity, float temperature, float soilMoisturePercent) {
  uint32_t currentMillis = millis();

  String ImageURL = "http://rangdong.com.vn/uploads/news/tin-san-pham/nguoi-ban-4.0-cua-nha-nong/smart-farm-rang-dong-nguoi-ban-4.0-cua-nha-nong-5.jpg";

  // Chỉ gửi cảnh báo nếu đã đủ 10 phút kể từ lần gửi trước
  if (currentMillis - lastAlertTime >= alertInterval) {
    bool alertSent = false;

    // Gửi cảnh báo Độ ẩm qua Telegram
    if (humidity < 30) {
      sendTelegramMessage("Cảnh báo: Độ ẩm quá thấp! (" + String(humidity) + "%)", ImageURL);
      alertSent = true;
    } else if (humidity > 80) {
      sendTelegramMessage("Cảnh báo: Độ ẩm quá cao! (" + String(humidity) + "%)", ImageURL);
      alertSent = true;
    }

    // Gửi cảnh báo Nhiệt độ qua Telegram
    if (temperature < 20) {
      sendTelegramMessage("Cảnh báo: Nhiệt độ quá thấp! (" + String(temperature) + "°C)", ImageURL);
      alertSent = true;
    } else if (temperature > 35) {
      sendTelegramMessage("Cảnh báo: Nhiệt độ quá cao! (" + String(temperature) + "°C)", ImageURL);
      alertSent = true;
    }

    // Gửi cảnh báo Độ ẩm đất qua Telegram
    if (soilMoisturePercent < 60) {
      sendTelegramMessage("Cảnh báo: Độ ẩm đất quá thấp! (" + String(soilMoisturePercent) + "%)", ImageURL);
      alertSent = true;
    } else if (soilMoisturePercent > 80) {
      sendTelegramMessage("Cảnh báo: Độ ẩm đất quá cao! (" + String(soilMoisturePercent) + "%)", ImageURL);
      alertSent = true;
    }

    // Nếu có cảnh báo được gửi, cập nhật lại thời gian gửi cuối cùng
    if (alertSent) {
      lastAlertTime = currentMillis;
    }
  }
}


// Hàm xử lý RPC thay đổi trạng thái LED
RPC_Response setLedSwitchState(const RPC_Data &data) {
  Serial.println("Nhận yêu cầu thay đổi trạng thái LED");
  bool LEDState = data;
  Serial.print("Bật công tắc đèn: ");
  Serial.println(LEDState ? "BẬT!" : "TẮT!");
  digitalWrite(LED_PIN, LEDState);
  attributesChanged = true;
  return RPC_Response("setValueButtonLED", LEDState);
}

RPC_Response setPumpState(const RPC_Data &data) {
  Serial.println("Nhận yêu cầu thay đổi trạng thái Máy bơm");
  bool newState = data; // Nhận trạng thái mong muốn từ dashboard

  // Nếu đang ở Auto mode, chuyển sang Manual mode
  if (isAutoMode) {
    isAutoMode = false;
    Serial.println("Chuyển sang Manual mode");
  }

  // Cập nhật trạng thái máy bơm
  pumpState = newState;
  digitalWrite(RELAY_PIN, pumpState);
  
  Serial.print("Máy bơm: ");
  Serial.println(pumpState ? "BẬT!" : "TẮT!");

  // Gửi trạng thái máy bơm lên ThingsBoard để đồng bộ
  tb.sendAttributeData("pumpState", pumpState);

  // Gửi thông báo đến Zalo
  String message = "Máy bơm của bạn đã " + String(pumpState ? "BẬT" : "TẮT") + "!";

  return RPC_Response("setValueButtonPump", pumpState);
}

RPC_Response getPumpState(const RPC_Data &data) {
  Serial.println("Yêu cầu lấy trạng thái Máy bơm");

  // Trả về trạng thái hiện tại của máy bơm
  return RPC_Response("getValueButtonPump", pumpState);
}

// Mảng chứa callback RPC
const std::array<RPC_Callback, 3U> callbacks = {
  RPC_Callback{ "setValueButtonLED", setLedSwitchState },
  RPC_Callback{ "setValueButtonPump", setPumpState },  // Thêm callback cho máy bơm
  RPC_Callback{ "getValueButtonPump", getPumpState }  // Thêm RPC get trạng thái máy bơm
};


// Prototype các task của FreeRTOS
void WiFiTask(void *pvParameters);
void ThingsBoardTask(void *pvParameters);
void SensorTask(void *pvParameters);
void ReconnectTask(void *pvParameters);
void TaskPumpControl(void *pvParameters);
void TaskButtonControl(void *pvParameters);
void TaskModeControl(void *pvParameters);


// Hàm setup
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


// Task kết nối WiFi
void WiFiTask(void *pvParameters) {
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connecting to WiFi...");
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(300 / portTICK_PERIOD_MS);
        Serial.print(".");
      }
      Serial.println("Connected to WiFi");
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);
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
        tb.sendAttributeData("pumpState", pumpState);
      }
      else {
        Serial.println("Failed to connect");
      }
    }
    // Nếu có thay đổi thuộc tính, cập nhật lên ThingsBoard
    if (attributesChanged) {
      attributesChanged = false;
      tb.sendAttributeData("ledState", digitalRead(LED_PIN));
    }
    tb.loop();
    vTaskDelay(800 / portTICK_PERIOD_MS);
  }
}


// Task thu thập dữ liệu cảm biến và gửi telemetry
void SensorTask(void *pvParameters) {
  for (;;) {
    if (millis() - previousDataSend > telemetrySendInterval) {
      previousDataSend = millis();
      
      // Đọc dữ liệu từ cảm biến DHT11
      float temperature = dht.readTemperature();
      float humidity = dht.readHumidity();
      // Đọc giá trị độ ẩm đất
      int soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);
      // Đảm bảo giá trị nằm trong khoảng [0, 4095] rồi mới map sang % (0 - 100)
      float soilMoisturePercent = map(soilMoistureRaw, 4095, 0, 0, 100);
      
      // In ra dữ liệu và hiển thị lên Serial Monitor
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

      checkAndSendAlerts(humidity, temperature, soilMoisturePercent);

      Serial.print("Sent GPS Data: Latitude: ");
      Serial.print(latitude);
      Serial.print(", Longitude: ");
      Serial.println(longitude);
    }
    vTaskDelay(telemetrySendInterval / portTICK_PERIOD_MS);
  }
}


// Task kiểm tra và kết nối lại WiFi và ThingsBoard
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


// Hàm bật/tắt máy bơm
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
      vTaskDelay(50 / portTICK_PERIOD_MS); // Nếu ở Manual mode thì chờ ngắn
    }
  }

  tb.sendAttributeData("pumpState", pumpState);
}

// Task điều khiển Pump bằng Button ở Manual mode
// void TaskButtonControl(void *pvParameters) {
//   bool lastButtonState = HIGH; // Lưu trạng thái trước của nút nhấn
  
//   for (;;) {
//     if (!isAutoMode) { // Chỉ chạy khi đang ở Manual Mode
//       bool currentButtonState = digitalRead(BUTTON_PIN);
//       // Phát hiện sự chuyển từ HIGH sang LOW (nút nhấn)
//       if (currentButtonState == LOW && lastButtonState == HIGH) { // Khi nút được nhấn
//         pumpState = !pumpState; // Đảo trạng thái máy bơm
//         turnOnOffPump(pumpState); // Bật hoặc tắt máy bơm
//         Serial.print("Máy bơm (Manual): ");
//         Serial.println(pumpState ? "Đang bật!" : "Đang tắt!");
//       }

//       lastButtonState = currentButtonState;
//     }
//     vTaskDelay(200 / portTICK_PERIOD_MS); // Debounce
//   }
// }

void TaskButtonControl(void *pvParameters) {
  bool lastButtonState = digitalRead(BUTTON_PIN);

  for (;;) {
    bool buttonState = digitalRead(BUTTON_PIN);

    if (buttonState == LOW && lastButtonState == HIGH) { // Nhấn nút
      vTaskDelay(50 / portTICK_PERIOD_MS); // Debounce
      if (digitalRead(BUTTON_PIN) == LOW) {
        if (isAutoMode) {
          isAutoMode = false; // Nếu đang Auto, chuyển sang Manual
          Serial.println("Chuyển sang Manual mode");
        }
        
        pumpState = !pumpState; // Đảo trạng thái máy bơm
        digitalWrite(RELAY_PIN, pumpState);
        Serial.print("Máy bơm: ");
        Serial.println(pumpState ? "BẬT!" : "TẮT!");

        // Gửi trạng thái mới lên ThingsBoard
        tb.sendAttributeData("pumpState", pumpState);
      }
    }

    lastButtonState = buttonState;
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  tb.sendAttributeData("pumpState", pumpState);
}


// Task xử lý Button chuyển đổi Auto/Manual mode
void TaskModeControl(void *pvParameters) {
  bool lastState = HIGH; // Trạng thái trước của button mode
  for (;;) {
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

// Hàm loop
void loop() {
  vTaskDelete(NULL);
}