#include "LEDTask/LEDTask.h"
#include "PumpTask/PumpTask.h"
#include "SensorTask/SensorTask.h"
#include "ConnectTask/ConnectTask.h"
#include "WeatherTask/WeatherTask.h"
#include "SendMessageTask.h"


// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
uint32_t lastAlertTime = 0;


// Gửi tin nhắn cảnh báo qua Telegram
void sendTelegramMessage(String message, String imageUrl) {
    String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + 
                "/sendPhoto?chat_id=" + String(TELEGRAM_CHAT_ID) + 
                "&photo=" + imageUrl + 
                "&caption=" + message;

    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        Serial.println("Gửi tin nhắn Telegram thành công!");
    } else {
        Serial.print("Lỗi gửi tin nhắn Telegram");
        Serial.println(httpResponseCode);
    }
    http.end();
}


void checkAndSendAlerts(float humidity, float temperature, float soilMoisturePercent) {
    uint32_t currentMillis = millis();

    String ImageURL = "http://rangdong.com.vn/uploads/news/tin-san-pham/nguoi-ban-4.0-cua-nha-nong/smart-farm-rang-dong-nguoi-ban-4.0-cua-nha-nong-5.jpg";

    // Chỉ gửi cảnh báo nếu đã đủ 10 phút kể từ lần gửi trước
    if (currentMillis - lastAlertTime >= alertInterval) {
        bool alertSent = false;

        String alertMessage = "🚨 **Cảnh báo Smart Farm** 🚨 %0A";
        // Gửi cảnh báo Độ ẩm qua Telegram
        if (humidity < 30) {
            alertMessage += "🌫 Độ ẩm quá thấp! (📉 " + String(humidity) + "%) %0A";
            alertSent = true;
        }  else if (humidity > 80) {
            alertMessage += "🌫 Độ ẩm quá cao! (📈 " + String(humidity) + "%) %0A";
            alertSent = true;
        }
        // Gửi cảnh báo Nhiệt độ qua Telegram
        if (temperature < 20) {
            alertMessage += "🌡️ Nhiệt độ quá thấp! (" + String(temperature) + "°C) %0A";
            alertSent = true;
        } else if (temperature > 35) {
            alertMessage += "🌡️ Nhiệt độ quá cao! (" + String(temperature) + "°C) %0A";
            alertSent = true;
        }
        // Gửi cảnh báo Độ ẩm đất qua Telegram
        if (soilMoisturePercent < 60) {
            alertMessage += "🌱 Độ ẩm đất thấp! (💧 " + String(soilMoisturePercent) + "%) %0A";
            alertSent = true;
        } else if (soilMoisturePercent > 80) {
            alertMessage += "🌱 Độ ẩm đất cao! (💧 " + String(soilMoisturePercent) + "%) %0A";
            alertSent = true;
        }
        // Gửi cảnh báo Mưa qua Telegram
        if (!checkRainNext12Hours()) {
            alertMessage += "Không có Mưa trong 12 giờ tới => Máy Bơm sẽ được BẬT!%0A";
            alertSent = true;
        } else {
            alertMessage += "Có Mưa trong 12 giờ tới => Máy Bơm sẽ được KHÔNG BẬT!%0A";
            alertSent = true;
        }

        sendTelegramMessage(alertMessage, ImageURL);

        // Nếu có cảnh báo được gửi, cập nhật lại thời gian gửi cuối cùng
        if (alertSent) {
            lastAlertTime = currentMillis;
        }
    }
}  