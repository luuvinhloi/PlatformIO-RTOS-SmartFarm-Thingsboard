#include <Arduino.h>
#include "ConnectTask/ConnectTask.h"
#include "LEDTask/LEDTask.h"
#include "PumpTask/PumpTask.h"
#include "SensorTask/SensorTask.h"
#include "LCDTask/LCDTask.h"
#include "SendMessageTask/SendMessageTask.h"


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

  // Khởi tạo LCD
  Wire.begin(SDA_PIN, SCL_PIN);
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