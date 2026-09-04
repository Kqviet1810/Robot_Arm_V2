#pragma once
#include <stdint.h>

// Trục chuyển động của cánh tay. X/Y/Z/A là stepper (qua TMC2209),
// B/C là servo SG90. AXIS_ALL dùng cho lệnh áp dụng mọi trục (vd Home All).
enum AxisId : uint8_t {
  AXIS_X = 0,
  AXIS_Y = 1,
  AXIS_Z = 2,
  AXIS_A = 3,   // trục ray (linear)
  AXIS_B = 4,   // servo xoay kẹp
  AXIS_C = 5,   // servo kẹp
  AXIS_COUNT = 6,
  AXIS_STEPPER_COUNT = 4,
  AXIS_ALL = 0xFF
};

// Trạng thái tổng của hệ thống, đồng bộ giữa Wroom (nguồn) và CYD (hiển thị).
enum RobotState : uint8_t {
  STATE_IDLE = 0,
  STATE_HOMING = 1,
  STATE_JOG = 2,
  STATE_TEACH_RECORD = 3,
  STATE_TEACH_PLAY = 4,
  STATE_ERROR = 5
};

// Định danh thiết bị dùng trong gói tin ESP-NOW (broadcast + lọc theo id).
enum DeviceId : uint8_t {
  DEV_WROOM = 1,
  DEV_C3 = 2,
  DEV_CYD = 3
};
