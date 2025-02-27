#include "ConnectTask/ConnectTask.h"
#include "PumpTask/PumpTask.h"
#include "SensorTask/SensorTask.h"
#include "SendMessageTask/SendMessageTask.h"
#include "LEDTask.h"


// Các biến trạng thái
volatile bool attributesChanged = false;
volatile bool ledState = false;     // Trạng thái của LED
bool lastLedButtonState = false;    // Trạng thái trước đó của nút nhấn

// Semaphore bảo vệ biến ledState
SemaphoreHandle_t ledSemaphore;


// Task điều khiển LED bằng nút nhấn
void TaskButtonLEDControl(void *pvParameters) {
    bool lastButtonState = digitalRead(LED_BUTTON_PIN);

    for (;;) {
        bool buttonState = digitalRead(LED_BUTTON_PIN);

        if (buttonState == LOW && lastButtonState == HIGH) { // Nhấn nút
            vTaskDelay(50 / portTICK_PERIOD_MS); // Debounce
            if (digitalRead(LED_BUTTON_PIN) == LOW) {  // Kiểm tra lại sau debounce

                ledState = !ledState; // Đảo trạng thái LED
                digitalWrite(LED_PIN, ledState);
                Serial.print("Đèn LED: ");
                Serial.println(ledState ? "BẬT!" : "TẮT!");

                // Kiểm tra trước khi gửi dữ liệu lên ThingsBoard
                if (tb.connected()) {
                    tb.sendAttributeData("getValueButtonLED", ledState);
                }
            }
        }

        lastButtonState = buttonState;
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Tránh CPU quá tải
    }
}

// Gửi trạng thái LED lên Dashboard mỗi 5 giây
void TaskSendLEDState(void *pvParameters) {
    for (;;) {
        // Chỉ gửi dữ liệu lên Dashboard khi có thay đổi trạng thái LED
        if (tb.connected()) {
            xSemaphoreTake(ledSemaphore, portMAX_DELAY);  // Đồng bộ với TaskReadButton

            static bool lastSentState = ledState;  // Lưu trạng thái LED đã gửi trước đó
            if (lastSentState != ledState) {  // Chỉ gửi khi trạng thái thay đổi
                tb.sendAttributeData("getValueButtonLED", ledState);  // Gửi trạng thái LED lên Dashboard
                Serial.println("Cập nhật trạng thái LED lên Dashboard!");
                lastSentState = ledState;  // Cập nhật trạng thái đã gửi
            }

            xSemaphoreGive(ledSemaphore);  // Giải phóng Semaphore
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Gửi mỗi 5 giây nếu có thay đổi
    }
}