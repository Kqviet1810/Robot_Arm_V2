#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <RobotArmProtocol.h>
#include "Config.h"
#include "MotorControl.h"
#include "ServoControl.h"
#include "LinkC3.h"
#include "LinkHMI.h"
#include "LinkWeb.h"
#include "TeachPlayback.h"

// Điều phối toàn hệ thống trên ESP32 Wroom: nhận lệnh từ HMI (CYD) qua
// ESP-NOW, điều khiển motor/servo, xử lý teach & playback, phát trạng thái
// định kỳ cho HMI.
class StateMachine {
 public:
  void begin();
  void update();

 private:
  MotorControl motors_;
  ServoControl servos_;
  LinkC3 linkC3_;
  LinkHMI linkHMI_;
  LinkWeb linkWeb_;
  TeachPlayback teach_;

  RobotState state_ = STATE_IDLE;
  uint8_t errorAxisMask_ = 0;

  uint32_t lastAnyJogCmdMs_ = 0;
  uint32_t lastStatusSendMs_ = 0;

  // Bộ đệm điểm cho RUN_CYCLE/RUN_QUEUE từ web — để làm biến thành viên
  // (không phải local trong hàm) nhằm tránh chiếm stack lớn (MAX_WAYPOINTS*6*4 byte).
  float webPointsBuf_[MAX_WAYPOINTS][AXIS_COUNT];

  void handleCommand(const CommandMsg &cmd);
  void handleWebCommand(const String &json);
  void checkJogWatchdog();
  void checkRuntimeFaults();
  void updateStateTransitions();
  void sendStatus();
};
