#pragma once
#include <Arduino.h>
#include <RobotArmProtocol.h>
#include "Config.h"

// Nhận AngleReport (góc AS5600 4 trục) từ ESP32 C3 qua UART2.
class LinkC3 {
 public:
  void begin() {
    serial_.setRxBufferSize(512);  // homing chạy blocking một lúc, tăng buffer tránh mất dữ liệu
    serial_.begin(C3_UART_BAUD, SERIAL_8N1, PIN_C3_UART_RX, PIN_C3_UART_TX);
  }

  // Gọi trong loop(): đọc hết byte đang chờ, cập nhật dữ liệu góc mới nhất.
  void update() {
    while (serial_.available()) {
      uint8_t b = (uint8_t)serial_.read();
      if (decoder_.feed(b) && decoder_.type == UART_MSG_ANGLE_REPORT &&
          decoder_.payloadLen == sizeof(AngleReportPayload)) {
        AngleReportPayload payload;
        memcpy(&payload, decoder_.payload, sizeof(payload));
        memcpy(latestAngleDeg_, payload.angleDeg, sizeof(latestAngleDeg_));
        latestSensorOk_ = payload.sensorOk;
        lastRxMs_ = millis();
        for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) updateUnwrap(i, payload.angleDeg[i]);
      }
    }
  }

  float angleDeg(uint8_t axisIdx) const { return latestAngleDeg_[axisIdx]; }
  uint8_t sensorOkMask() const { return latestSensorOk_; }
  bool isLinkFresh(uint32_t maxAgeMs = 500) const { return (millis() - lastRxMs_) <= maxAgeMs; }

  // Góc trục động cơ đã "gỡ vòng lặp" (unwrap): AS5600 gắn ở đuôi động cơ chỉ
  // đọc được góc trong 1 vòng quay (0-360°), trong khi động cơ có thể quay
  // nhiều vòng (X/Y/Z qua tỉ số truyền 4.5:1, A qua nhiều vòng dọc ray).
  // Giá trị này cộng dồn độ lệch giữa các lần đọc liên tiếp (~50Hz từ C3) để
  // suy ra góc động cơ tuyệt đối liên tục — dùng làm mốc tin cậy khi thả trơn
  // (freewheel) lúc cầm tay chỉ việc, nơi bộ đếm bước của stepper không còn
  // đúng. Yêu cầu tốc độ quay giữa 2 lần đọc không vượt quá nửa vòng (điều
  // kiện luôn thoả khi di chuyển bằng tay).
  float unwrappedMotorDeg(uint8_t axisIdx) const { return unwrapped_[axisIdx]; }

 private:
  HardwareSerial serial_{2};
  UartFrameDecoder decoder_;
  float latestAngleDeg_[AXIS_STEPPER_COUNT] = {0};
  uint8_t latestSensorOk_ = 0;
  uint32_t lastRxMs_ = 0;

  float unwrapped_[AXIS_STEPPER_COUNT] = {0, 0, 0, 0};
  float prevRaw_[AXIS_STEPPER_COUNT] = {0, 0, 0, 0};
  bool unwrapInit_[AXIS_STEPPER_COUNT] = {false, false, false, false};

  void updateUnwrap(uint8_t axisIdx, float rawDeg) {
    if (!unwrapInit_[axisIdx]) {
      unwrapped_[axisIdx] = rawDeg;
      prevRaw_[axisIdx] = rawDeg;
      unwrapInit_[axisIdx] = true;
      return;
    }
    float delta = rawDeg - prevRaw_[axisIdx];
    if (delta > 180.0f) delta -= 360.0f;
    else if (delta < -180.0f) delta += 360.0f;
    unwrapped_[axisIdx] += delta;
    prevRaw_[axisIdx] = rawDeg;
  }
};
