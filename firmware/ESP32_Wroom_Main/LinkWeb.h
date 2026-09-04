#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "Config.h"

// Phục vụ giao diện web (LittleFS, thư mục data/ upload sẵn) + WebSocket
// JSON thời gian thực tại "/ws". ESP32 chạy WiFi SoftAP (IP mặc định
// 192.168.4.1) để thiết bị điều khiển kết nối trực tiếp, không cần
// Internet, và tránh lỗi "mixed content" của trình duyệt (trang phục vụ
// cùng origin http:// với WebSocket ws://, không phải wss://).
//
// Cùng pattern hàng đợi vòng (ring buffer) single-producer/single-consumer
// như LinkHMI.h: callback WebSocket chạy trên task async_tcp riêng, không
// xử lý motor trực tiếp ở đó — chỉ đẩy chuỗi JSON thô vào hàng đợi, xử lý
// thật trong StateMachine::update() ở loop() chính.
class LinkWeb {
 public:
  bool begin() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
    Serial.print("[Wroom] SoftAP IP: ");
    Serial.println(WiFi.softAPIP());

    if (!LittleFS.begin(true)) {
      Serial.println("[Wroom] Canh bao: khong mount duoc LittleFS -> khong phuc vu duoc giao dien web (van dieu khien duoc qua CYD)");
    } else {
      server_.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    }

    ws_.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                        void *arg, uint8_t *data, size_t len) {
      onWsEvent(server, client, type, arg, data, len);
    });
    server_.addHandler(&ws_);
    server_.begin();
    return true;
  }

  // Gọi mỗi vòng loop(): dọn client WebSocket đã ngắt kết nối.
  void update() { ws_.cleanupClients(); }

  // Trả về true và đổ nội dung vào `out` nếu có bản tin JSON thô mới trong hàng đợi.
  bool popCommand(String &out) {
    if (head_ == tail_) return false;
    out = queue_[tail_];
    tail_ = (uint8_t)((tail_ + 1) % kQueueSize);
    return true;
  }

  void broadcastStatus(const String &json) { ws_.textAll(json); }

 private:
  static const uint8_t kQueueSize = 4;
  AsyncWebServer server_{80};
  AsyncWebSocket ws_{"/ws"};
  String queue_[kQueueSize];
  volatile uint8_t head_ = 0;
  volatile uint8_t tail_ = 0;

  void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                 void *arg, uint8_t *data, size_t len) {
    (void)server;
    (void)client;
    if (type != WS_EVT_DATA) return;
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    // Chỉ xử lý frame text hoàn chỉnh, không phân mảnh (đủ cho các gói JSON
    // nhỏ ở đây; bỏ qua binary/frame phân mảnh để giữ logic đơn giản).
    if (!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) return;

    uint8_t nextHead = (uint8_t)((head_ + 1) % kQueueSize);
    if (nextHead == tail_) return;  // hàng đợi đầy, bỏ qua lệnh mới nhất để không chặn task async_tcp
    queue_[head_] = String((const char *)data, len);
    head_ = nextHead;
  }
};
