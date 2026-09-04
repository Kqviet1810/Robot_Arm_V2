#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <RobotArmProtocol.h>
#include "Config.h"

// ESP-NOW: gửi CommandMsg tới Wroom, nhận StatusMsg (từ Wroom) và
// TelemetryMsg (từ C3). Dùng broadcast, không cần pair MAC thủ công.
class LinkWroom {
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

  void sendCommand(const CommandMsg &cmd) {
    esp_now_send(kBroadcastAddr, (const uint8_t *)&cmd, sizeof(cmd));
  }

  const StatusMsg &status() const { return status_; }
  const TelemetryMsg &telemetry() const { return telemetry_; }
  bool wroomLinkFresh() const { return (millis() - lastStatusMs_) <= LINK_FRESH_TIMEOUT_MS; }
  bool c3LinkFresh() const { return (millis() - lastTelemetryMs_) <= LINK_FRESH_TIMEOUT_MS; }

 private:
  static constexpr uint8_t kBroadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static LinkWroom *self_;

  StatusMsg status_;
  TelemetryMsg telemetry_;
  uint32_t lastStatusMs_ = 0;
  uint32_t lastTelemetryMs_ = 0;

  // Chữ ký callback esp_now_recv_cb_t theo ESP32 Arduino core >= 2.0.5 (IDF
  // >= 4.4). Nếu dùng core cũ hơn, đổi tham số đầu thành `const uint8_t *mac`.
  static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    (void)info;
    if (!self_) return;
    if (len == sizeof(StatusMsg)) {
      StatusMsg msg;
      memcpy(&msg, data, sizeof(msg));
      if (msg.deviceId == DEV_WROOM && msg.msgType == ESPNOW_MSG_STATUS) {
        self_->status_ = msg;
        self_->lastStatusMs_ = millis();
      }
    } else if (len == sizeof(TelemetryMsg)) {
      TelemetryMsg msg;
      memcpy(&msg, data, sizeof(msg));
      if (msg.deviceId == DEV_C3 && msg.msgType == ESPNOW_MSG_TELEMETRY) {
        self_->telemetry_ = msg;
        self_->lastTelemetryMs_ = millis();
      }
    }
  }
};
