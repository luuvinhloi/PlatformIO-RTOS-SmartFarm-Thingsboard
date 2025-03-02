#ifndef LED_TASK_H
#define LED_TASK_H


#include <ArduinoOTA.h>
#include <Arduino.h>


// Định nghĩa chân kết nối
#define LED_PIN 2 // Chân kết nối LED
#define LED_BUTTON_PIN 4 // Button LED


// Các thuộc tính ThingsBoard
constexpr char LED_STATE_ATTR[] = "ledState";


// Các biến trạng thái
extern volatile bool attributesChanged;
extern volatile bool ledState;   
extern bool lastLedButtonState;  


// Semaphore bảo vệ biến ledState
extern SemaphoreHandle_t ledSemaphore;


constexpr std::array<const char *, 1U> SHARED_ATTRIBUTES_LIST = {
    LED_STATE_ATTR
};


void TaskButtonLEDControl(void*);
void TaskSendLEDState(void*);


#endif