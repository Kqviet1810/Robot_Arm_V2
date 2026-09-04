#pragma once
#include <Arduino.h>
#include <RobotArmProtocol.h>
#include "Config.h"
#include "MotorControl.h"
#include "ServoControl.h"
#include "LinkC3.h"
#include "LinkHMI.h"
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
  TeachPlayback teach_;

  RobotState state_ = STATE_IDLE;
  uint8_t errorAxisMask_ = 0;

  uint32_t lastAnyJogCmdMs_ = 0;
  uint32_t lastStatusSendMs_ = 0;

  void handleCommand(const CommandMsg &cmd);
  void checkJogWatchdog();
  void checkRuntimeFaults();
  void updateStateTransitions();
  void sendStatus();
};
