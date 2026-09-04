#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <RobotArmProtocol.h>
#include "Config.h"

// Broadcast TelemetryMsg (góc AS5600 thô) qua ESP-NOW để màn hình CYD
// hiển thị giám sát. Không cần pair MAC: dùng địa chỉ broadcast chung.
class LinkHMI {
 public:
  bool begin() {
    WiFi.mode(WIFI_STA);
    // Pin đúng kênh WiFi mà ESP32 Wroom dùng cho SoftAP, nếu không ESP-NOW
    // có thể không nhận được gói tin (STA không kết nối AP nào nên tự trôi
    // kênh nếu không set tay).
    esp_wifi_set_channel(WIFI_AP_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (esp_now_init() != ESP_OK) return false;

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, kBroadcastAddr, 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (!esp_now_is_peer_exist(kBroadcastAddr)) {
      esp_now_add_peer(&peer);
    }
    return true;
  }

  void sendTelemetry(const float angleDeg[AXIS_STEPPER_COUNT], uint8_t sensorOk) {
    TelemetryMsg msg;
    msg.seq = seq_++;
    memcpy(msg.angleDeg, angleDeg, sizeof(msg.angleDeg));
    msg.sensorOk = sensorOk;
    esp_now_send(kBroadcastAddr, (const uint8_t *)&msg, sizeof(msg));
  }

 private:
  static constexpr uint8_t kBroadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint32_t seq_ = 0;
};

constexpr uint8_t LinkHMI::kBroadcastAddr[6];
