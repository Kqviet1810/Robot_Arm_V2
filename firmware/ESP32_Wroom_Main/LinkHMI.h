#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <RobotArmProtocol.h>

// ESP-NOW: nhận CommandMsg từ CYD (HMI), gửi StatusMsg cho CYD.
// Dùng broadcast, không cần pair MAC thủ công.
//
// Callback esp_now chạy trong task WiFi nội bộ (không phải ISR cứng) nên
// dùng hàng đợi vòng (ring buffer) đơn giản single-producer/single-consumer
// để chuyển lệnh sang loop() chính một cách an toàn, không cần mutex.
class LinkHMI {
 public:
  bool begin() {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return false;

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, kBroadcastAddr, 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (!esp_now_is_peer_exist(kBroadcastAddr)) {
      esp_now_add_peer(&peer);
    }

    self_ = this;
    esp_now_register_recv_cb(onRecv);
    return true;
  }

  // Trả về true và đổ dữ liệu vào `out` nếu có lệnh mới trong hàng đợi.
  bool popCommand(CommandMsg &out) {
    if (head_ == tail_) return false;
    out = queue_[tail_];
    tail_ = (uint8_t)((tail_ + 1) % kQueueSize);
    return true;
  }

  void sendStatus(const StatusMsg &msg) {
    esp_now_send(kBroadcastAddr, (const uint8_t *)&msg, sizeof(msg));
  }

 private:
  static const uint8_t kQueueSize = 8;
  static constexpr uint8_t kBroadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  static LinkHMI *self_;
  CommandMsg queue_[kQueueSize];
  volatile uint8_t head_ = 0;
  volatile uint8_t tail_ = 0;

  // Chữ ký callback esp_now_recv_cb_t theo ESP32 Arduino core >= 2.0.5 (IDF
  // >= 4.4). Nếu dùng core cũ hơn, đổi tham số đầu thành `const uint8_t *mac`.
  static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    (void)info;
    if (!self_ || len != sizeof(CommandMsg)) return;
    CommandMsg msg;
    memcpy(&msg, data, sizeof(msg));
    if (msg.deviceId != DEV_CYD || msg.msgType != ESPNOW_MSG_COMMAND) return;

    uint8_t nextHead = (uint8_t)((self_->head_ + 1) % kQueueSize);
    if (nextHead == self_->tail_) return;  // hàng đợi đầy, bỏ qua lệnh cũ nhất còn chờ
    self_->queue_[self_->head_] = msg;
    self_->head_ = nextHead;
  }
};
