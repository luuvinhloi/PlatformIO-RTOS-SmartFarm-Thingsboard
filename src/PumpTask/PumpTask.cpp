#include "PumpTask.h"
#include "ConnectTask/ConnectTask.h"
#include "LEDTask/LEDTask.h"
#include "SensorTask/SensorTask.h"
#include "SendMessageTask/SendMessageTask.h"
#include "WeatherTask/WeatherTask.h"

#define SOIL_MOISTURE_THRESHOLD_LOW 60  // Ngưỡng dưới độ ẩm đất (%)
#define SOIL_MOISTURE_THRESHOLD_HIGH 80 // Ngưỡng trên độ ẩm đất (%)

// Các biến trạng thái
bool isAutoMode = true;     // Auto Mode
bool pumpState = false;     // Trạng thái của Pump
static int lastPumpState = 0;   // Biến lưu trạng thái cũ
volatile bool attributesChangedPump = false;

// Biến toàn cục
RTC_DS3231 rtc;

// Cập nhật trạng thái LED lên Dashboard
void updateDashboardPumpState() {
    if (tb.connected()) {
        tb.sendAttributeData("getValueButtonPump", pumpState);
        attributesChangedPump = false;  // Reset trạng thái thay đổi
    }
}

// Xử lý RPC từ Dashboard để thay đổi Pump
RPC_Response setPumpState(const RPC_Data &data) {
    bool newState = data; // Nhận trạng thái mong muốn từ Dashboard

    isAutoMode = !newState;
    pumpState = newState;

    // Cập nhật trạng thái bơm và relay
    digitalWrite(RELAY_PIN, pumpState);

    Serial.printf("Dashboard yêu cầu: %s MÁY BƠM!\n", pumpState ? "BẬT" : "TẮT");
    Serial.println(isAutoMode ? "Máy Bơm ở Auto Mode!" : "Máy Bơm ở Manual Mode!");

    attributesChangedPump = true;

    // Gửi trạng thái mới lên ThingsBoard để đồng bộ
    return RPC_Response("setValueButtonPump", pumpState);
}

// RPC để Dashboard lấy trạng thái Pump
RPC_Response getPumpState(const RPC_Data &data) {
    return RPC_Response("getValueButtonPump", pumpState);
}

// Task 1: Theo dõi cảm biến mưa
void TaskRainSensorMonitor(void *pvParameters) {
    for (;;) {
        // bool isRaining = digitalRead(RAIN_SENSOR_PIN) == LOW; // LOW nghĩa là có mưa
        bool isRaining = 0; // LOW nghĩa là có mưa
        if (isRaining && pumpState) {
            Serial.println("Phát hiện có mưa! Tắt máy bơm.");
            pumpState = false;
            digitalWrite(RELAY_PIN, pumpState);
            updateDashboardPumpState();
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// Task 2: Điều khiển bơm theo lịch tưới hằng ngày
void TaskSchedulePump(void *pvParameters) {
    for (;;) {
        DateTime now = rtc.now();  // Lấy thời gian hiện tại
        bool rainExpected = checkRainNext12Hours();
        // Kiểm tra cảm biến mưa
        // bool isRaining = digitalRead(RAIN_SENSOR_PIN) == LOW; // LOW nghĩa là có mưa
        bool isRaining = 0; // LOW nghĩa là có mưa

        if (!rainExpected && !isRaining) {  // Nếu không có mưa dự báo hoặc hiện tại không có mưa
            if ((now.hour() == 8 && now.minute() == 30) && !pumpState) {
                Serial.printf("Thời gian hiện tại: %02d:%02d\n", now.hour(), now.minute());
                Serial.println("BẬT máy bơm theo lịch tưới hằng ngày.");
                pumpState = true;
            } else if ((now.hour() == 9 && now.minute() == 45) && pumpState) {
                Serial.printf("Thời gian hiện tại: %02d:%02d\n", now.hour(), now.minute());
                Serial.println("TẮT máy bơm theo lịch tưới hằng ngày.");
                pumpState = false;
            }

            // Gửi trạng thái máy bơm nếu thay đổi
            if (pumpState != lastPumpState) {
                digitalWrite(RELAY_PIN, pumpState);
                updateDashboardPumpState();
                lastPumpState = pumpState;
            }
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Cập nhật sau mỗi 5 giây
    }
}

// Task 3: Điều khiển bơm theo độ ẩm đất (Ưu tiên cao hơn)
void TaskSoilMoisturePump(void *pvParameters) {
    float soilMoisture;
    for (;;) {
        if (xQueueReceive(soilMoistureQueue, &soilMoisture, portMAX_DELAY) == pdTRUE) {
            bool isRaining = 0;
            bool newPumpState = pumpState;

            if ((soilMoisture < SOIL_MOISTURE_THRESHOLD_LOW) && !isRaining) {
                Serial.println("Độ ẩm đất thấp, BẬT máy bơm!");
                newPumpState = true;
            } else if ((soilMoisture > SOIL_MOISTURE_THRESHOLD_HIGH) || isRaining) {
                Serial.println("Độ ẩm đất đạt mức yêu cầu, TẮT máy bơm!");
                newPumpState = false;
            }

            // Chỉ bật/tắt máy bơm nếu trạng thái thay đổi
            if (newPumpState != pumpState) {
                pumpState = newPumpState;
                digitalWrite(RELAY_PIN, pumpState);
                // Gửi trạng thái máy bơm nếu thay đổi
                updateDashboardPumpState();
                lastPumpState = pumpState;
                // Gửi cảnh báo nếu máy bơm được bật
                checkAndSendAlertsPump(soilMoisture, pumpState);
            }
        }
    }
}

// Task 4: Điều khiển Pump bằng nút nhấn
void TaskButtonPumpControl(void *pvParameters) {
    bool lastButtonState = digitalRead(BUTTON_PIN);

    for (;;) {
        bool buttonState = digitalRead(BUTTON_PIN);

        if (buttonState == LOW && lastButtonState == HIGH) { // Nhấn nút
            vTaskDelay(50 / portTICK_PERIOD_MS); // Debounce
            
            if (digitalRead(BUTTON_PIN) == LOW) {  // Kiểm tra lại sau debounce
                // Chỉ cho phép bật/tắt máy bơm khi ở chế độ manual
                if (!isAutoMode) {  
                    pumpState = !pumpState;
                    digitalWrite(RELAY_PIN, pumpState);
                    Serial.printf("Máy bơm: %s\n", pumpState ? "BẬT!" : "TẮT!");
                    // Gửi trạng thái máy bơm nếu thay đổi
                    updateDashboardPumpState();
                } else {
                    Serial.println("Chế độ Auto - Không thể bật/tắt thủ công.");
                }
            }
        }

        lastButtonState = buttonState;
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Tránh CPU quá tải
    }
}

// Task 5: Xử lý Button chuyển đổi Auto/Manual mode
void TaskModeControl(void *pvParameters) {
    attachInterrupt(digitalPinToInterrupt(MODE_BUTTON_PIN), []() {
        isAutoMode = !isAutoMode;
        Serial.printf("Chế độ: %s\n", isAutoMode ? "Auto" : "Manual");
    }, FALLING);

    vTaskDelete(NULL);  // Không cần vòng lặp, xử lý bằng interrupt
}