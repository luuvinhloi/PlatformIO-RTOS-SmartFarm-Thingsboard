#include "LEDTask.h"
#include "ConnectTask/ConnectTask.h"
#include "PumpTask/PumpTask.h"
#include "SensorTask/SensorTask.h"
#include "SendMessageTask/SendMessageTask.h"

// Các biến trạng thái
volatile bool attributesChangedLED = false;
volatile bool ledState = false;     // Trạng thái của LED
static int lastLEDState = 0;   // Biến lưu trạng thái cũ
bool lastLedButtonState = false;    // Trạng thái trước đó của nút nhấn

// Semaphore bảo vệ biến ledState
SemaphoreHandle_t ledSemaphore;

// Biến toàn cục
RTC_DS3231 rtcLED;

// Cập nhật trạng thái LED lên Dashboard
void updateDashboardLEDState() {
    if (tb.connected()) {
        tb.sendAttributeData("getValueButtonLED", ledState);
        attributesChangedLED = false;  // Reset trạng thái thay đổi
    }
}

// Xử lý RPC từ Dashboard để thay đổi LED
RPC_Response setLedSwitchState(const RPC_Data &data) {
    bool newLEDState = data;
    
    if (ledState != newLEDState) {  // Chỉ thay đổi nếu có sự khác biệt
        xSemaphoreTake(ledSemaphore, portMAX_DELAY);
        ledState = newLEDState;
        digitalWrite(LED_PIN, ledState);
        xSemaphoreGive(ledSemaphore);

        Serial.printf("Dashboard yêu cầu: %s LED!\n", ledState ? "BẬT" : "TẮT");
        attributesChangedLED = true;
    }
    return RPC_Response("setValueButtonLED", ledState);
}

// RPC để Dashboard lấy trạng thái LED
RPC_Response getLedState(const RPC_Data &data) {
    return RPC_Response("getValueButtonLED", ledState);
}

// Task 1: Điều khiển LED theo lịch hằng ngày
void TaskScheduleLED(void *pvParameters) {
    for (;;) {
        DateTime now = rtcLED.now();  // Lấy thời gian hiện tại

        if ((now.hour() == 18 && now.minute() == 00) && !ledState) {
            Serial.printf("Thời gian hiện tại: %02d:%02d\n", now.hour(), now.minute());
            Serial.println("BẬT LED theo lịch hằng ngày.");
            ledState = true;
        } else if ((now.hour() == 6 && now.minute() == 0) && ledState) {
            Serial.printf("Thời gian hiện tại: %02d:%02d\n", now.hour(), now.minute());
            Serial.println("TẮT LED theo lịch hằng ngày.");
            ledState = false;
        }

        // Gửi trạng thái LED nếu thay đổi
        if (ledState != lastLEDState) {
            digitalWrite(LED_PIN, ledState);
            updateDashboardLEDState();
            lastLEDState = ledState;
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Cập nhật sau mỗi 5 giây
    }
}

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
                Serial.printf("Đèn LED: %s\n", ledState ? "BẬT" : "TẮT");
                updateDashboardLEDState();
            }
        }

        lastButtonState = buttonState;
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Tránh CPU quá tải
    }
}

// Gửi trạng thái LED lên Dashboard mỗi 3 giây
void TaskSendLEDState(void *pvParameters) {
    bool lastSentState = ledState;  // Lưu trạng thái đã gửi

    for (;;) {
        if (attributesChangedLED && tb.connected()) {  
            xSemaphoreTake(ledSemaphore, portMAX_DELAY);
            if (lastSentState != ledState) {
                updateDashboardLEDState();
                lastSentState = ledState;
            }
            xSemaphoreGive(ledSemaphore);
            attributesChangedLED = false;  // Reset trạng thái thay đổi
        }
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}