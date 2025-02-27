#include "ConnectTask/ConnectTask.h"
#include "LEDTask/LEDTask.h"
#include "SensorTask/SensorTask.h"
#include "SendMessageTask/SendMessageTask.h"
#include "PumpTask.h"


// Các biến trạng thái
bool isAutoMode = true;     // Auto Mode
bool pumpState = false;     // Trạng thái của Pump


// Hàm bật/tắt máy bơm
void turnOnOffPump(int onOff) {
    digitalWrite(RELAY_PIN, onOff);
}


// Task điều khiển Pump auto mode
static int lastPumpState = -1; // Biến lưu trạng thái cũ

void TaskPumpControl(void *pvParameters) {
    while(1) {
        if (isAutoMode) {
            turnOnOffPump(1);
            Serial.println("Máy bơm (Auto): Đang bật!");
            vTaskDelay(10000 / portTICK_PERIOD_MS);

            turnOnOffPump(0);
            Serial.println("Máy bơm (Auto): Đang tắt!");
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        } else {
            vTaskDelay(50 / portTICK_PERIOD_MS);
        }

        // Chỉ gửi dữ liệu nếu trạng thái thay đổi
        if (pumpState != lastPumpState) {
            tb.sendAttributeData("getValueButtonPump", pumpState);
            lastPumpState = pumpState;
        }
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