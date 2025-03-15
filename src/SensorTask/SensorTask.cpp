#include "SensorTask.h"
#include "ConnectTask/ConnectTask.h"
#include "LEDTask/LEDTask.h"
#include "PumpTask/PumpTask.h"
#include "SendMessageTask/SendMessageTask.h"
#include "LCDTask/LCDTask.h"

// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
uint32_t previousDataSend = 0;

// Khởi tạo các đối tượng
DHT dht(DHT_PIN, DHT11);

QueueHandle_t soilMoistureQueue = xQueueCreate(10, sizeof(float));  // Tạo queue chứa 10 giá trị float

// Toạ độ GPS của thiết bị
constexpr float latitude = 10.898576;
constexpr float longitude = 106.794645;

void updateSoilMoisture(float moisture) {
    xQueueSend(soilMoistureQueue, &moisture, portMAX_DELAY);
}

// Task thu thập dữ liệu cảm biến và gửi telemetry
void SensorTask(void *pvParameters) {
    static float lastTemp = -100, lastHumidity = -100, lastSoilMoisture = -100;

    for (;;) {
        uint32_t currentMillis = millis();

        if (currentMillis - previousDataSend > telemetrySendInterval) {
            previousDataSend = currentMillis;
            
            // Đọc dữ liệu từ cảm biến DHT11
            float temperature = dht.readTemperature();
            float humidity = dht.readHumidity();

            // Nếu cảm biến lỗi, bỏ qua lần gửi dữ liệu này
            if (isnan(temperature) || isnan(humidity)) {
                vTaskDelay(2000 / portTICK_PERIOD_MS); // Tăng thời gian chờ
                continue; // Bỏ qua nếu có lỗi
            }

            // Đọc giá trị độ ẩm đất
            int soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);
            float soilMoisturePercent = 0.0;
            // Nếu cảm biến bị ngắt kết nối hoặc lỗi
            if (soilMoistureRaw >= 4000) {  
                Serial.println("Cảnh báo: Cảm biến độ ẩm không kết nối hoặc lỗi!");
                soilMoisturePercent = -1; // Gán giá trị -1 để báo lỗi
            } else {
                // Chuyển đổi giá trị ADC sang phần trăm độ ẩm
                float soilMoisturePercent = (4095.0 - soilMoistureRaw) / 4095.0 * 100.0;
                // Đảm bảo giá trị nằm trong khoảng hợp lệ [0, 100]
                soilMoisturePercent = constrain(soilMoisturePercent, 0.0, 100.0);
            }
            
            // Chỉ cập nhật LCD nếu dữ liệu thay đổi
            if (temperature != lastTemp || humidity != lastHumidity || soilMoisturePercent != lastSoilMoisture) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.printf("T:%.1fC H:%.1f%%", temperature, humidity);
                lcd.setCursor(0, 1);
                lcd.printf("Soil: %.1f%%", soilMoisturePercent);
                
                lastTemp = temperature;
                lastHumidity = humidity;
                lastSoilMoisture = soilMoisturePercent;
            }

            // Lưu trữ giá trị trước đó
            struct TelemetryData {
                const char* key;
                float value;
                float lastValue; // Thêm biến lưu giá trị trước đó
            };

            // Khởi tạo giá trị trước đó (giả định không có giá trị nào được gửi ban đầu)
            static TelemetryData telemetry[] = {
                {"temperature", -100, NAN},
                {"humidity", -100, NAN},
                {"soil_moisture", -100, NAN},
                {"lat", latitude, NAN},
                {"long", longitude, NAN}
            };

            telemetry[0].value = temperature;
            telemetry[1].value = humidity;
            telemetry[2].value = soilMoisturePercent;

            // Gửi dữ liệu chỉ khi giá trị thay đổi
            for (auto& data : telemetry) {
                // Nếu giá trị thay đổi (hoặc lần đầu tiên NAN), thì gửi
                if (isnan(data.lastValue) || data.value != data.lastValue) {
                    tb.sendTelemetryData(data.key, data.value);
                    data.lastValue = data.value; // Cập nhật giá trị trước đó
                }
            }

            // In ra dữ liệu và hiển thị lên Serial Monitor
            Serial.printf("Temperature: %.2f°C, Humidity: %.2f%%, Soil Moisture: %.2f%%\n", temperature, humidity, soilMoisturePercent);
            
            // Cập nhật giá trị độ ẩm đất gửi sang PumpTask module
            updateSoilMoisture(soilMoisturePercent);

            //  Gửi cảnh báo qua Telegram
            checkAndSendAlerts(humidity, temperature, soilMoisturePercent);
        }
        
        vTaskDelay(telemetrySendInterval / portTICK_PERIOD_MS);
    }
}