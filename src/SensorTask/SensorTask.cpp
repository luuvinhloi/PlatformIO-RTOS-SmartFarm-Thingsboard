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


QueueHandle_t soilMoistureQueue = xQueueCreate(10, sizeof(float));  // Tạo queue chứa 10 giá trị float


// Toạ độ GPS của thiết bị
constexpr float latitude = 10.898576;
constexpr float longitude = 106.794645;


void updateSoilMoisture(float humidity) {
    float moisture = humidity;
    xQueueSend(soilMoistureQueue, &moisture, portMAX_DELAY);
}


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
            float soilMoisturePercent = 0.0;
            // Nếu cảm biến bị ngắt kết nối hoặc lỗi
            if (soilMoistureRaw >= 4000) {  
                Serial.println("🚨 Cảnh báo: Cảm biến độ ẩm không kết nối hoặc lỗi!");
                soilMoisturePercent = -1; // Gán giá trị -1 để báo lỗi
            } else {
                // Chuyển đổi giá trị ADC sang phần trăm độ ẩm
                float soilMoisturePercent = (4095.0 - soilMoistureRaw) / 4095.0 * 100.0;
                
                // Đảm bảo giá trị nằm trong khoảng hợp lệ [0, 100]
                soilMoisturePercent = constrain(soilMoisturePercent, 0.0, 100.0);
            }
            
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
            // Cập nhật giá trị độ ẩm đất gửi sang PumpTask module
            updateSoilMoisture(soilMoisturePercent);

            // Gửi dữ liệu GPS lên ThingsBoard
            tb.sendTelemetryData("lat", latitude);
            tb.sendTelemetryData("long", longitude);

            Serial.print("Sent GPS Data: Latitude: ");
            Serial.print(latitude);
            Serial.print(", Longitude: ");
            Serial.println(longitude);

            //  Gửi cảnh báo qua Telegram
            checkAndSendAlerts(humidity, temperature, soilMoisturePercent);
        }
        
        vTaskDelay(telemetrySendInterval / portTICK_PERIOD_MS);
    }
}