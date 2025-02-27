#ifndef PUMP_TASK_H
#define PUMP_TASK_H


#include <ArduinoOTA.h>
#include <Arduino.h>


// Định nghĩa chân kết nối
#define RELAY_PIN 26		// Relay
#define BUTTON_PIN 16      	// Button Pump
#define MODE_BUTTON_PIN 17 	// Button Auto/Manual mode


// Các biến trạng thái
extern bool isAutoMode;    	// Auto Mode
extern bool pumpState;     	// Trạng thái của Pump


void TaskPumpControl(void *pvParameters);
void TaskButtonPumpControl(void *pvParameters);
void TaskModeControl(void *pvParameters);


#endif
