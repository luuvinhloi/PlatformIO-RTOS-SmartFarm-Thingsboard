#include "ConnectTask/ConnectTask.h"
#include "LEDTask/LEDTask.h"
#include "SensorTask/SensorTask.h"
#include "SendMessageTask/SendMessageTask.h"
#include "WeatherTask/WeatherTask.h"
#include "PumpTask.h"


#define SOIL_MOISTURE_THRESHOLD_LOW 60  // Ngưỡng dưới độ ẩm đất (%)
#define SOIL_MOISTURE_THRESHOLD_HIGH 80 // Ngưỡng trên độ ẩm đất (%)

// Các biến trạng thái
bool isAutoMode = true;     // Auto Mode
bool pumpState = false;     // Trạng thái của Pump
// bool lastPumpState = false;

// Biến toàn cục
RTC_DS3231 rtc;

// Hàm bật/tắt máy bơm
void turnOnOffPump(int onOff) {
    digitalWrite(RELAY_PIN, onOff);
}

// Task điều khiển Pump auto mode
static int lastPumpState = 0; // Biến lưu trạng thái cũ

// void TaskPumpControl(void *pvParameters) {
//     for (;;) {
//         if (isAutoMode) {
//             if (pumpState == 0) {
//                 turnOnOffPump(1);
//                 pumpState = 1;
//                 Serial.println("Máy bơm (Auto): Đang bật!");
//                 tb.sendAttributeData("getValueButtonPump", pumpState);
//                 vTaskDelay(10000 / portTICK_PERIOD_MS);
//             } else if (pumpState == 1) {
//                 turnOnOffPump(0);
//                 pumpState = 0;
//                 Serial.println("Máy bơm (Auto): Đang tắt!");
//                 tb.sendAttributeData("getValueButtonPump", pumpState);
//                 vTaskDelay(5000 / portTICK_PERIOD_MS);
//             }
//         } else {
//             vTaskDelay(50 / portTICK_PERIOD_MS);
//         }
//     }
// }


// Task điều khiển Pump Auto Mode
void TaskPumpControl(void *pvParameters) {
	float soilMoisture;

    for(;;) {
        DateTime now = rtc.now();  // Lấy thời gian hiện tại

        // In thời gian hiện tại ra Serial
        Serial.print("Thời gian hiện tại: ");
        Serial.print(now.hour());
        Serial.print(":");
        Serial.println(now.minute());

        bool rainExpected = checkRainNext12Hours();

        // Ưu tiên 1: Kiểm tra độ ẩm đất
        if (xQueueReceive(soilMoistureQueue, &soilMoisture, portMAX_DELAY) == pdTRUE) {
            if (soilMoisture < SOIL_MOISTURE_THRESHOLD_LOW) {
                Serial.printf("Soil Moisture: %.2f%%\n", soilMoisture);
                Serial.println("Độ ẩm đất thấp, BẬT máy bơm!");
                turnOnOffPump(1);
                pumpState = true;
            } else if (soilMoisture > SOIL_MOISTURE_THRESHOLD_HIGH) {
                Serial.printf("Soil Moisture: %.2f%%\n", soilMoisture);
                Serial.println("Độ ẩm đất đạt mức yêu cầu, TẮT máy bơm!");
                turnOnOffPump(0);
                pumpState = false;
            }
        }
        
        // Ưu tiên 2: Tưới nước theo lịch hằng ngày từ 8h - 9h30, trừ khi có mưa
        if ((now.hour() >= 9 && now.minute() >= 10) && !pumpState) {
            Serial.println("Bật máy bơm theo lịch tưới hằng ngày.");
            turnOnOffPump(1);
            pumpState = true;
        } else if ((now.hour() >= 9 && now.minute() >= 15) && !pumpState) {
            Serial.println("Tắt máy bơm theo lịch hằng ngày.");
            turnOnOffPump(0);
            pumpState = false;
        }

        // Gửi trạng thái máy bơm nếu thay đổi
        if (pumpState != lastPumpState) {
            tb.sendAttributeData("getValueButtonPump", pumpState);
            lastPumpState = pumpState;
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Cập nhật sau mỗi 5 giây
    }
}


// Task điều khiển Pump bằng nút nhấn
void TaskButtonPumpControl(void *pvParameters) {
    bool lastButtonState = digitalRead(BUTTON_PIN);

    for (;;) {
        bool buttonState = digitalRead(BUTTON_PIN);

        if (buttonState == LOW && lastButtonState == HIGH) { // Nhấn nút
            vTaskDelay(50 / portTICK_PERIOD_MS); // Debounce
            if (digitalRead(BUTTON_PIN) == LOW) {  // Kiểm tra lại sau debounce
                if (isAutoMode) {
                    isAutoMode = false;
                    Serial.println("Chuyển sang Manual mode");
                }

                pumpState = !pumpState; // Đảo trạng thái máy bơm
                digitalWrite(RELAY_PIN, pumpState);
                Serial.print("Máy bơm: ");
                Serial.println(pumpState ? "BẬT!" : "TẮT!");
                tb.sendAttributeData("getValueButtonPump", pumpState);

                // Kiểm tra trước khi gửi dữ liệu lên ThingsBoard
                if (tb.connected()) {
                    tb.sendAttributeData("getValueButtonPump", pumpState);
                }
            }
        }

        lastButtonState = buttonState;
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Tránh CPU quá tải
    }
}


// Task xử lý Button chuyển đổi Auto/Manual mode
void TaskModeControl(void *pvParameters) {
    bool lastState = HIGH; // Trạng thái trước của button mode
    for (;;) {
        bool currentState = digitalRead(MODE_BUTTON_PIN);
        if (currentState == LOW && lastState == HIGH) { // Button mode được nhấn
            isAutoMode = !isAutoMode; // Chuyển đổi mode
            Serial.print("Chế độ: ");
            Serial.println(isAutoMode ? "Auto" : "Manual");
        }
        lastState = currentState;
        vTaskDelay(100 / portTICK_PERIOD_MS); // Tránh lặp lại nhanh
    }
}