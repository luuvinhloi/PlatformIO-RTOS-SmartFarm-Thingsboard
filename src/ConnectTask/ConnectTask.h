#ifndef CONNECT_TASK_H
#define CONNECT_TASK_H


#include <ArduinoOTA.h>
#include <Arduino_MQTT_Client.h>
#include <ArduinoHttpClient.h>
#include <ThingsBoard.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <array>


// Khởi tạo các đối tượng
extern WiFiClient wifiClient;
extern Arduino_MQTT_Client mqttClient;
extern ThingsBoard tb;


// Prototype các task của FreeRTOS
void WiFiTask(void *pvParameters);
void ThingsBoardTask(void *pvParameters);
void ReconnectTask(void *pvParameters);

#endif