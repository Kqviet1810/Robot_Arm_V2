#pragma once
#include <stdint.h>
#include "RobotArmCommon.h"

// ============================================================================
// Giao thức ESP-NOW: Wroom <-> CYD (HMI) và C3 -> CYD (telemetry).
// Dùng địa chỉ broadcast (FF:FF:FF:FF:FF:FF) cho mọi gói tin, không cần pair
// MAC thủ công. Mỗi struct tự mang deviceId (nguồn gửi) + msgType để bên
// nhận lọc/định tuyến; esp_now_send() truyền thẳng struct packed dưới dạng
// mảng byte.
// ============================================================================

enum EspNowMsgType : uint8_t {
  ESPNOW_MSG_TELEMETRY = 0x10,  // C3 -> CYD
  ESPNOW_MSG_STATUS = 0x11,     // Wroom -> CYD
  ESPNOW_MSG_COMMAND = 0x12     // CYD -> Wroom
};

// Lệnh điều khiển gửi từ HMI tới Wroom (CommandMsg.cmd).
enum RobotCommand : uint8_t {
  CMD_NONE = 0,
  CMD_JOG_START = 1,      // axis, dir (+1/-1), speedPct
  CMD_JOG_STOP = 2,       // axis (AXIS_ALL = dừng mọi jog)
  CMD_HOME_AXIS = 3,      // axis
  CMD_HOME_ALL = 4,
  CMD_SERVO_SET = 5,      // axis (B/C), value = góc đích (độ)
  CMD_TEACH_START = 6,    // axis = AXIS_ALL hoặc trục cụ thể để freewheel
  CMD_TEACH_STOP = 7,
  CMD_TEACH_SAVE_POINT = 8,
  CMD_TEACH_SAVE_PROGRAM = 9,   // programName
  CMD_TEACH_CLEAR = 10,
  CMD_PLAY_PROGRAM = 11,        // programName
  CMD_PLAY_PAUSE = 12,
  CMD_PLAY_STOP = 13,
  CMD_PLAY_LOOP_TOGGLE = 14,
  CMD_ESTOP = 15,
  CMD_CLEAR_ERROR = 16
};

// C3 -> CYD: dữ liệu góc thô AS5600, tần số thấp, chỉ để giám sát trên HMI.
struct __attribute__((packed)) TelemetryMsg {
  uint8_t deviceId = DEV_C3;
  uint8_t msgType = ESPNOW_MSG_TELEMETRY;
  uint32_t seq = 0;
  float angleDeg[AXIS_STEPPER_COUNT] = {0};
  uint8_t sensorOk = 0;  // bitmask, bit i = AS5600 kênh i hợp lệ
};

// Wroom -> CYD: trạng thái điều khiển hiện tại, hiển thị trên các màn hình.
struct __attribute__((packed)) StatusMsg {
  uint8_t deviceId = DEV_WROOM;
  uint8_t msgType = ESPNOW_MSG_STATUS;
  uint32_t seq = 0;
  uint8_t state = STATE_IDLE;           // RobotState
  float axisPos[AXIS_COUNT] = {0};      // X,Y,Z,A: độ/mm theo trục; B,C: độ servo
  uint8_t axisFault = 0;                // bitmask lỗi/stall theo AxisId (stepper)
  int16_t sgResult[AXIS_STEPPER_COUNT] = {0};  // StallGuard SG_RESULT hiện tại
  uint8_t waypointCount = 0;            // số điểm đã ghi trong phiên teach hiện tại
  uint8_t playIndex = 0;                // waypoint đang chạy khi ở STATE_TEACH_PLAY
  uint8_t playTotal = 0;
  uint8_t loopEnabled = 0;
};

// CYD -> Wroom: lệnh điều khiển từ HMI.
struct __attribute__((packed)) CommandMsg {
  uint8_t deviceId = DEV_CYD;
  uint8_t msgType = ESPNOW_MSG_COMMAND;
  uint32_t seq = 0;
  uint8_t cmd = CMD_NONE;      // RobotCommand
  uint8_t axis = AXIS_ALL;     // AxisId (khi cần)
  int8_t dir = 0;              // +1 / -1 / 0 (dùng cho JOG_START)
  uint8_t speedPct = 100;      // 1-100%
  float value = 0.0f;          // tham số chung (góc servo, ...)
  char programName[16] = {0};  // dùng cho SAVE_PROGRAM / PLAY_PROGRAM
};
