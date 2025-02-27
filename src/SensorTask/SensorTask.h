#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <ArduinoOTA.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include <array>


// Định nghĩa chân kết nối
#define DHT_PIN 14  // Chân kết nối cảm biến nhiệt độ độ ẩm DHT11
#define SOIL_MOISTURE_PIN 34  // Chân kết nối cảm biến độ ẩm đất


// Các hằng số cấu hình
constexpr int16_t telemetrySendInterval = 10000U; // 10 giây sensor modlue


// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
extern uint32_t previousDataSend;

extern DHT dht;


// Prototype các task của FreeRTOS
void SensorTask(void *pvParameters);


#endif
