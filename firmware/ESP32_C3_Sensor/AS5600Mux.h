#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <RobotArmProtocol.h>
#include "Config.h"

// Đọc 4 cảm biến AS5600 gắn sau 4 kênh của TCA9548A (I2C mux).
// Driver tự viết, chỉ dùng các thanh ghi cần thiết của AS5600 (giữ nhẹ,
// tránh phụ thuộc thư viện lớn không cần thiết):
//   - STATUS (0x0B): bit3 MD (magnet detected), bit4 ML (quá yếu), bit5 MH (quá mạnh)
//   - RAW ANGLE (0x0C/0x0D): góc thô 12-bit (0-4095), chưa lọc/hysteresis.
class AS5600Mux {
 public:
  bool begin() {
    Wire.beginTransmission(TCA9548A_ADDR);
    return Wire.endTransmission() == 0;
  }

  // Đọc góc (độ, 0-360) trục `axisIdx` (0..3 = X,Y,Z,A). Trả về true nếu
  // đọc thành công và cảm biến báo phát hiện nam châm hợp lệ.
  bool readAxisDeg(uint8_t axisIdx, float &outDeg) {
    if (!selectChannel(AS5600_CHANNEL[axisIdx])) return false;

    uint8_t status = 0;
    if (!readReg8(AS5600_STATUS_REG, status)) return false;
    bool magnetDetected = (status & AS5600_STATUS_MD_BIT) != 0;

    uint16_t raw = 0;
    if (!readReg16(AS5600_RAW_ANGLE_REG, raw)) return false;
    raw &= 0x0FFF;  // 12-bit

    outDeg = (raw * 360.0f) / 4096.0f;
    return magnetDetected;
  }

 private:
  static const uint8_t AS5600_STATUS_REG = 0x0B;
  static const uint8_t AS5600_RAW_ANGLE_REG = 0x0C;
  static const uint8_t AS5600_STATUS_MD_BIT = 0x08;  // bit3 = MD trên datasheet AS5600

  bool selectChannel(uint8_t channel) {
    Wire.beginTransmission(TCA9548A_ADDR);
    Wire.write((uint8_t)(1 << channel));
    return Wire.endTransmission() == 0;
  }

  bool readReg8(uint8_t reg, uint8_t &value) {
    Wire.beginTransmission(AS5600_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)AS5600_ADDR, 1) != 1) return false;
    value = Wire.read();
    return true;
  }

  bool readReg16(uint8_t reg, uint16_t &value) {
    Wire.beginTransmission(AS5600_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)AS5600_ADDR, 2) != 2) return false;
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    value = ((uint16_t)hi << 8) | lo;
    return true;
  }
};
