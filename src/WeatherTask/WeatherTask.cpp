#include "WeatherTask.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// API Key từ OpenWeatherMap
const char* apiKey = "8b579de0fc93996bc8c50ccb1a5795ec";    // API Key OPENWeather DỰ BÁO MƯA
const char* lat = "10.898576"; 
const char* lon = "106.794645";


bool checkRainNext12Hours() {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + String(lat) + "&lon=" + String(lon) + "&appid=" + String(apiKey) + "&units=metric";
    
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
        String response = http.getString();
        // Serial.println(response);

        // Phân tích JSON
        DynamicJsonDocument doc(10240);
        deserializeJson(doc, response);
        
        JsonArray list = doc["list"];
        
        for (int i = 0; i < 4; i++) {  // Kiểm tra 12 giờ tới (mỗi entry = 3 giờ)
            float rain = list[i]["rain"]["3h"] | 0;  
            if (rain > 0) {
                return true;
            }
        }
    } else {
        Serial.println("Lỗi lấy dữ liệu thời tiết!");
    }

    http.end();
    return false;
}