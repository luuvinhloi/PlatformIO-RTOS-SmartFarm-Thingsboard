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
#define SOIL_MOISTURE_PIN 33  // Chân kết nối cảm biến độ ẩm đất 34
// #define RAIN_SENSOR_PIN 27  // Chân kết nối cảm biến mưa

// Các hằng số cấu hình
constexpr int16_t telemetrySendInterval = 10000U; // 10 giây sensor modlue

// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
extern uint32_t previousDataSend;

extern DHT dht;

extern QueueHandle_t soilMoistureQueue;  // Hàng đợi để truyền dữ liệu
void updateSoilMoisture();

// Prototype các task của FreeRTOS
void SensorTask(void *pvParameters);


#endif
