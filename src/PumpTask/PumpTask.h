#ifndef PUMP_TASK_H
#define PUMP_TASK_H

#include <ArduinoOTA.h>
#include <Arduino.h>
#include <time.h>
#include "RTClib.h"  // Thư viện RTC để kiểm tra thời gian
#include "ConnectTask/ConnectTask.h"

// Định nghĩa chân kết nối
#define RELAY_PIN 13        // Relay
#define BUTTON_PIN 16       // Button Pump
#define MODE_BUTTON_PIN 17 	// Button Auto/Manual mode

// Các biến trạng thái
extern bool isAutoMode;    	// Auto Mode
extern bool pumpState;     	// Trạng thái của Pump

extern volatile bool attributesChangedPump;

extern RTC_DS3231 rtc;

RPC_Response setPumpState(const RPC_Data &data);
RPC_Response getPumpState(const RPC_Data &data);

void TaskRainSensorMonitor(void *pvParameters);
void TaskSchedulePump(void *pvParameters);
void TaskSoilMoisturePump(void *pvParameters);
void TaskButtonPumpControl(void *pvParameters);
void TaskModeControl(void *pvParameters);

#endif