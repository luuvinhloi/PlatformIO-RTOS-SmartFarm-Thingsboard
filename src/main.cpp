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
void TaskButtonLEDControl(void *pvParameters);
void TaskSendLEDState(void *pvParameters);
void TaskPumpControl(void *pvParameters);
void TaskButtonPumpControl(void *pvParameters);
void TaskModeControl(void *pvParameters);


// Hàm setup
void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);

  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_BUTTON_PIN, INPUT_PULLUP);

  // Khởi tạo Semaphore
  ledSemaphore = xSemaphoreCreateMutex();

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

  char buffer[20];  // Khai báo biến buffer

  if (rtc.lostPower()) {
    // Serial.println("RTC mất nguồn, đặt lại thời gian!");
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Cập nhật theo thời gian biên dịch

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
  xTaskCreate(WiFiTask, "WiFiTask", 4096, NULL, 1, NULL);
  xTaskCreate(ThingsBoardTask, "ThingsBoardTask", 4096, NULL, 1, NULL);
  xTaskCreate(ReconnectTask, "ReconnectTask", 4096, NULL, 1, NULL);
  xTaskCreate(SensorTask, "SensorTask", 4096, NULL, 1, NULL);
  xTaskCreate(TaskButtonLEDControl, "TaskButtonLEDControl", 4096, NULL, 1, NULL);
  xTaskCreate(TaskSendLEDState, "TaskSendLEDState", 2048, NULL, 1, NULL);
  xTaskCreate(TaskPumpControl, "TaskPumpControl", 4096, NULL, 1, NULL);
  xTaskCreate(TaskButtonPumpControl, "TaskButtonPumpControl", 4096, NULL, 1, NULL);
  xTaskCreate(TaskModeControl, "TaskModeControl", 4096, NULL, 1, NULL);
}


// Hàm loop
void loop() {
  vTaskDelete(NULL);
}