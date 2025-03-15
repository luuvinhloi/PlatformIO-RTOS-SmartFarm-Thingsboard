#ifndef LED_TASK_H
#define LED_TASK_H

#include <ArduinoOTA.h>
#include <Arduino.h>
#include <time.h>
#include "RTClib.h"  // Thư viện RTC để kiểm tra thời gian
#include "ConnectTask/ConnectTask.h"

// Định nghĩa chân kết nối
#define LED_PIN 18 // Chân kết nối LED - Chân cũ 12
#define LED_BUTTON_PIN 4 // Button LED - Chân cũ 4

// Các thuộc tính ThingsBoard
constexpr char LED_STATE_ATTR[] = "ledState";

// Các biến trạng thái
extern volatile bool attributesChangedLED;
extern volatile bool ledState;   
extern bool lastLedButtonState;  

// Semaphore bảo vệ biến ledState
extern SemaphoreHandle_t ledSemaphore;

extern RTC_DS3231 rtc;

constexpr std::array<const char *, 1U> SHARED_ATTRIBUTES_LIST = {
    LED_STATE_ATTR
};

RPC_Response setLedSwitchState(const RPC_Data &data);
RPC_Response getLedState(const RPC_Data &data);

void updateDashboardLEDState();
void TaskScheduleLED(void *pvParameters);
void TaskButtonLEDControl(void*);
void TaskSendLEDState(void*);

#endif