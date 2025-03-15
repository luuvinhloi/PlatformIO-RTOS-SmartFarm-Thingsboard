#include <Arduino.h>
#include <time.h>
#include "ConnectTask/ConnectTask.h"
#include "LEDTask/LEDTask.h"
#include "PumpTask/PumpTask.h"
#include "SensorTask/SensorTask.h"
#include "LCDTask/LCDTask.h"
#include "SendMessageTask/SendMessageTask.h"
#include "WeatherTask/WeatherTask.h"


// Các hằng số cấu hình
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;


// Prototype các task của FreeRTOS
void WiFiTask(void *pvParameters);
void ThingsBoardTask(void *pvParameters);
void ReconnectTask(void *pvParameters);
void SensorTask(void *pvParameters);
void TaskScheduleLED(void *pvParameters);
void TaskButtonLEDControl(void *pvParameters);
void TaskSendLEDState(void *pvParameters);
void TaskRainSensorMonitor(void *pvParameters);
void TaskSchedulePump(void *pvParameters);
void TaskSoilMoisturePump(void *pvParameters);
void TaskButtonPumpControl(void *pvParameters);
void TaskModeControl(void *pvParameters);

// Hàm setup
void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);

  // Khởi tạo Semaphore
  ledSemaphore = xSemaphoreCreateMutex();

  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_BUTTON_PIN, INPUT_PULLUP);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);

  dht.begin();

  Wire.begin(SDA_PIN, SCL_PIN);

  // **Khởi tạo RTC**
  if (!rtc.begin()) {
    Serial.println("Không tìm thấy RTC, kiểm tra kết nối!");
    while (1); // Dừng chương trình nếu RTC không hoạt động
  }

  if (rtc.lostPower()) {
    char buffer[20];  // Khai báo biến buffer
    Serial.println("RTC mất nguồn, nhập thời gian thực theo định dạng YYYY MM DD HH MM SS:");
    while (!Serial.available());  // Chờ người dùng nhập thời gian

    int year, month, day, hour, minute, second;
    Serial.readStringUntil('\n').toCharArray(buffer, 20); // Đọc dữ liệu nhập vào
    sscanf(buffer, "%d %d %d %d %d %d", &year, &month, &day, &hour, &minute, &second);
    
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    Serial.println("Đã cập nhật thời gian RTC!");
  }

  Serial.println("RTC đã được khởi tạo thành công!");

  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();  // Bật đèn nền
  lcd.setCursor(0, 0);
  lcd.print("Smart Farm!");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  // Khởi tạo các task của FreeRTOS
  xTaskCreate(WiFiTask, "WiFiTask", 8192, nullptr, 1, nullptr);
  xTaskCreate(ThingsBoardTask, "ThingsBoardTask", 8192, nullptr, 1, nullptr);
  xTaskCreate(ReconnectTask, "ReconnectTask", 8192, nullptr, 1, nullptr);
  xTaskCreate(SensorTask, "SensorTask", 4096, nullptr, 1, nullptr);
  xTaskCreate(TaskScheduleLED, "LED Schedule", 4096, nullptr, 1, nullptr);
  xTaskCreate(TaskButtonLEDControl, "TaskButtonLEDControl", 4096, nullptr, 1, nullptr);
  xTaskCreate(TaskSendLEDState, "TaskSendLEDState", 2048, nullptr, 1, nullptr);
  xTaskCreate(TaskRainSensorMonitor, "Rain Sensor", 1024, nullptr, 1, nullptr);
  xTaskCreate(TaskSoilMoisturePump, "Soil Moisture Pump", 2048, nullptr, 1, nullptr);
  xTaskCreate(TaskSchedulePump, "Pump Schedule", 4096, nullptr, 1, nullptr);
  xTaskCreate(TaskButtonPumpControl, "TaskButtonPumpControl", 4096, nullptr, 1, nullptr);
  xTaskCreate(TaskModeControl, "TaskModeControl", 4096, nullptr, 1, nullptr);
}

// Hàm loop
void loop() {
  vTaskDelete(nullptr);
}