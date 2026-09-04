// ESP32_C3_Sensor — Robot Arm V2
// Board: ESP32 C3 Super Mini
// Vai trò: đọc 4 cảm biến AS5600 (qua TCA9548A) và phát dữ liệu góc:
//   - UART tới ESP32 Wroom (Main), tần số cao, dùng cho homing/teach/playback.
//   - ESP-NOW broadcast tới ESP32 CYD (HMI), tần số thấp, chỉ để giám sát.
//
// Yêu cầu thư viện: RobotArmProtocol (xem firmware/README.md để cài đặt).

#include <Wire.h>
#include <RobotArmProtocol.h>
#include "Config.h"
#include "AS5600Mux.h"
#include "LinkWroom.h"
#include "LinkHMI.h"

AS5600Mux sensors;
LinkWroom linkWroom;
LinkHMI linkHMI;

float axisAngleDeg[AXIS_STEPPER_COUNT] = {0};
uint8_t axisSensorOk = 0;

uint32_t lastAngleReportMs = 0;
uint32_t lastTelemetryMs = 0;

void readAllAxes() {
  uint8_t okMask = 0;
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    float deg = 0;
    bool ok = sensors.readAxisDeg(i, deg);
    axisAngleDeg[i] = deg;
    if (ok) okMask |= (1 << i);
  }
  axisSensorOk = okMask;
}

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);
  if (!sensors.begin()) {
    Serial.println("[C3] Canh bao: khong thay TCA9548A tren bus I2C");
  }

  linkWroom.begin();
  if (!linkHMI.begin()) {
    Serial.println("[C3] Loi khoi tao ESP-NOW");
  }

  Serial.println("[C3] Sensor node san sang");
}

void loop() {
  readAllAxes();

  uint32_t now = millis();

  if (now - lastAngleReportMs >= ANGLE_REPORT_INTERVAL_MS) {
    lastAngleReportMs = now;
    linkWroom.sendAngleReport(axisAngleDeg, axisSensorOk);
  }

  if (now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryMs = now;
    linkHMI.sendTelemetry(axisAngleDeg, axisSensorOk);
  }
}
