// ESP32_Wroom_Main — Robot Arm V2
// Board: ESP32 Wroom (Main Controller)
// Vai trò: điều khiển 4 stepper (TMC2209 qua UART, StallGuard homing) +
// 2 servo SG90; nhận góc cảm biến từ ESP32 C3 qua UART; nhận lệnh và gửi
// trạng thái tới ESP32 CYD (HMI) qua ESP-NOW; xử lý "cầm tay chỉ việc"
// (teach & playback).
//
// Yêu cầu thư viện: RobotArmProtocol (nội bộ), TMCStepper, FastAccelStepper,
// ESP32Servo. Xem firmware/README.md để biết chi tiết cài đặt & sơ đồ chân.

#include <RobotArmProtocol.h>
#include "StateMachine.h"

StateMachine robot;

void setup() {
  Serial.begin(115200);
  robot.begin();
}

void loop() {
  robot.update();
}
