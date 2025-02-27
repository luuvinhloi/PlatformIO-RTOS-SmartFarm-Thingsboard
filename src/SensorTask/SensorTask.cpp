#include "ConnectTask/ConnectTask.h"
#include "LEDTask/LEDTask.h"
#include "PumpTask/PumpTask.h"
#include "SensorTask.h"
#include "SendMessageTask/SendMessageTask.h"
#include "LCDTask/LCDTask.h"


// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
uint32_t previousDataSend;


// Khởi tạo các đối tượng
DHT dht(DHT_PIN, DHT11);


// Toạ độ GPS của thiết bị
constexpr float latitude = 10.898576;
constexpr float longitude = 106.794645;


// Task thu thập dữ liệu cảm biến và gửi telemetry
void SensorTask(void *pvParameters) {
    for (;;) {
        if (millis() - previousDataSend > telemetrySendInterval) {
            previousDataSend = millis();
            
            // Đọc dữ liệu từ cảm biến DHT11
            float temperature = dht.readTemperature();
            float humidity = dht.readHumidity();

            if (isnan(temperature) || isnan(humidity)) {
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                temperature = dht.readTemperature();
                humidity = dht.readHumidity();
            }

            // Đọc giá trị độ ẩm đất
            int soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);
            // Đảm bảo giá trị nằm trong khoảng [0, 4095] rồi mới map sang % (0 - 100)
            float soilMoisturePercent = map(soilMoistureRaw, 4095, 0, 0, 100);
            
            // In ra dữ liệu và hiển thị lên Serial Monitor
            Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%, Soil Moisture: %.2f%%\n", temperature, humidity, soilMoisturePercent);
            
            // Hiển thị dữ liệu lên LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.printf("T:%.1fC H:%.1f%%", temperature, humidity);
            lcd.setCursor(0, 1);
            lcd.printf("Soil: %.1f%%", soilMoisturePercent);
            
            // Gửi dữ liệu lên ThingsBoard
            if (!isnan(temperature) && !isnan(humidity)) {
                tb.sendTelemetryData("temperature", temperature);
                tb.sendTelemetryData("humidity", humidity);
            }
            tb.sendTelemetryData("soil_moisture", soilMoisturePercent);

            // Gửi dữ liệu GPS lên ThingsBoard
            tb.sendTelemetryData("lat", latitude);
            tb.sendTelemetryData("long", longitude);

            Serial.print("Sent GPS Data: Latitude: ");
            Serial.print(latitude);
            Serial.print(", Longitude: ");
            Serial.println(longitude);

            checkAndSendAlerts(humidity, temperature, soilMoisturePercent);
        }
        
        vTaskDelay(telemetrySendInterval / portTICK_PERIOD_MS);
    }
}