#ifndef SEND_MESSAGE_TASK_H
#define SEND_MESSAGE_TASK_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <vector>

// Telegram Bot
#define TELEGRAM_BOT_TOKEN "8122936180:AAHVsRVDZrjwevWBwJnzZOHm_dgcJgkevuY"
#define TELEGRAM_CHAT_ID "1378242143"

// Các hằng số cấu hình
constexpr uint32_t alertInterval = 10000; // 10 giây (10.000 milliseconds)
// constexpr uint32_t alertInterval = 600000; // 10 phút (600.000 milliseconds)

// Lưu thời điểm gửi dữ liệu và kiểm tra kết nối
extern uint32_t lastAlertTime;

void sendTelegramMessage(String message, String imageUrl);
void checkAndSendAlerts(float humidity, float temperature, float soilMoisturePercent);
void checkAndSendAlertsPump(float soilMoisturePercent, bool pumpState);

#endif
