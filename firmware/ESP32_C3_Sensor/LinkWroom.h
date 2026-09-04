#pragma once
#include <Arduino.h>
#include <RobotArmProtocol.h>
#include "Config.h"

// Gửi AngleReport định kỳ tới ESP32 Wroom qua UART (HardwareSerial1).
// C3 chỉ đóng vai trò nguồn dữ liệu cảm biến -> không cần giải mã ở đây,
// giữ interface đơn giản nhất có thể.
class LinkWroom {
 public:
  void begin() {
    serial_.begin(UART_BAUD, SERIAL_8N1, PIN_UART_RX, PIN_UART_TX);
  }

  void sendAngleReport(const float angleDeg[AXIS_STEPPER_COUNT], uint8_t sensorOk) {
    AngleReportPayload payload;
    payload.seq = seq_++;
    memcpy(payload.angleDeg, angleDeg, sizeof(payload.angleDeg));
    payload.sensorOk = sensorOk;

    uint8_t frame[sizeof(AngleReportPayload) + 5];
    uint8_t len = uartEncodeFrame(UART_MSG_ANGLE_REPORT, (const uint8_t *)&payload, sizeof(payload), frame);
    if (len > 0) serial_.write(frame, len);
  }

 private:
  HardwareSerial serial_{1};
  uint32_t seq_ = 0;
};
