#include "LEDTask/LEDTask.h"
#include "PumpTask/PumpTask.h"
#include "SensorTask/SensorTask.h"
#include "ConnectTask/ConnectTask.h"
#include "WeatherTask/WeatherTask.h"
#include "SendMessageTask.h"


// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
uint32_t lastAlertTime = 0;

// Biến lưu trạng thái dự báo mưa trước đó
bool lastRainExpected = true;


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


void appendAlertMessage(String &message, const String &text, bool &alertSent) {
    message += text + " %0A";
    alertSent = true;
}


void checkAndSendAlerts(float humidity, float temperature, float soilMoisturePercent) {
    uint32_t currentMillis = millis();
    if (currentMillis - lastAlertTime < alertInterval) return; // Chỉ gửi cảnh báo sau mỗi 10 phút

    bool rainExpected = checkRainNext12Hours();
    bool alertSent = false;
    String alertMessage = "🚨 **Cảnh báo Smart Farm** 🚨 %0A";
    String imageURL = "http://rangdong.com.vn/uploads/news/tin-san-pham/nguoi-ban-4.0-cua-nha-nong/smart-farm-rang-dong-nguoi-ban-4.0-cua-nha-nong-5.jpg";

    // Danh sách điều kiện cảnh báo
    std::vector<std::pair<String, bool>> conditions = {
        {"🌫 Độ ẩm quá thấp! (📉 " + String(humidity) + "%)", humidity < 30},
        {"🌫 Độ ẩm quá cao! (📈 " + String(humidity) + "%)", humidity > 80},
        {"🌡️ Nhiệt độ quá thấp! (" + String(temperature) + "°C)", temperature < 20},
        {"🌡️ Nhiệt độ quá cao! (" + String(temperature) + "°C)", temperature > 35},
        {"🌱 Độ ẩm đất thấp! (💧 " + String(soilMoisturePercent) + "%)", soilMoisturePercent < 60},
        {"🌱 Độ ẩm đất cao! (💧 " + String(soilMoisturePercent) + "%)", soilMoisturePercent > 80}
    };

    // Duyệt qua danh sách điều kiện và thêm thông báo nếu cần
    for (const auto &condition : conditions) {
        if (condition.second) appendAlertMessage(alertMessage, condition.first, alertSent);
    }

    // Kiểm tra thay đổi trạng thái mưa
    if (rainExpected != lastRainExpected) {
        appendAlertMessage(alertMessage, rainExpected ? "🌧 Dự báo Có Mưa trong 12 giờ tới => Máy Bơm sẽ KHÔNG BẬT!" : "🌤 Dự báo Không có Mưa trong 12 giờ tới => Máy Bơm sẽ được BẬT TỰ ĐỘNG!", alertSent);
        lastRainExpected = rainExpected;
    }

    // Gửi cảnh báo nếu có bất kỳ điều kiện nào đúng
    if (alertSent) {
        sendTelegramMessage(alertMessage, imageURL);
        lastAlertTime = currentMillis;
    }
}