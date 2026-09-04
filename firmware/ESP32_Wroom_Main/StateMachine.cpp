#include "StateMachine.h"

void StateMachine::begin() {
  motors_.begin();
  servos_.begin();
  linkC3_.begin();
  teach_.begin();
  // LinkWeb phải begin() TRƯỚC LinkHMI: LinkWeb set WiFi.mode(WIFI_AP) +
  // kênh cố định, ESP-NOW (LinkHMI) chạy tiếp trên nền AP đó.
  linkWeb_.begin();
  if (!linkHMI_.begin()) {
    Serial.println("[Wroom] Loi khoi tao ESP-NOW");
  }
  Serial.println("[Wroom] Main controller san sang");
}

void StateMachine::update() {
  motors_.update();
  linkC3_.update();
  linkWeb_.update();
  teach_.update(motors_, servos_);

  CommandMsg cmd;
  while (linkHMI_.popCommand(cmd)) handleCommand(cmd);

  String webJson;
  while (linkWeb_.popCommand(webJson)) handleWebCommand(webJson);

  checkJogWatchdog();
  checkRuntimeFaults();
  updateStateTransitions();

  uint32_t now = millis();
  if (now - lastStatusSendMs_ >= STATUS_SEND_INTERVAL_MS) {
    lastStatusSendMs_ = now;
    sendStatus();
  }
}

void StateMachine::handleCommand(const CommandMsg &cmd) {
  switch (cmd.cmd) {
    case CMD_JOG_START:
      if (cmd.axis < AXIS_STEPPER_COUNT && state_ != STATE_TEACH_RECORD &&
          state_ != STATE_TEACH_PLAY && state_ != STATE_ERROR) {
        motors_.startJog(cmd.axis, cmd.dir, cmd.speedPct);
        state_ = STATE_JOG;
        lastAnyJogCmdMs_ = millis();
      }
      break;

    case CMD_JOG_STOP:
      if (cmd.axis == AXIS_ALL) motors_.stopAllJog();
      else if (cmd.axis < AXIS_STEPPER_COUNT) motors_.stopJog(cmd.axis);
      break;

    case CMD_HOME_AXIS:
      if (cmd.axis < AXIS_STEPPER_COUNT && state_ != STATE_TEACH_RECORD && state_ != STATE_TEACH_PLAY) {
        state_ = STATE_HOMING;
        bool ok = motors_.homeAxis(cmd.axis);
        if (!ok) errorAxisMask_ |= (1 << cmd.axis);
        state_ = ok ? STATE_IDLE : STATE_ERROR;
      }
      break;

    case CMD_HOME_ALL:
      if (state_ != STATE_TEACH_RECORD && state_ != STATE_TEACH_PLAY) {
        state_ = STATE_HOMING;
        bool allOk = true;
        for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
          if (!motors_.homeAxis(i)) {
            allOk = false;
            errorAxisMask_ |= (1 << i);
          }
        }
        state_ = allOk ? STATE_IDLE : STATE_ERROR;
      }
      break;

    case CMD_SERVO_SET:
      if (cmd.axis == AXIS_B) servos_.setDeg(0, cmd.value);
      else if (cmd.axis == AXIS_C) servos_.setDeg(1, cmd.value);
      break;

    case CMD_TEACH_START:
      if (state_ == STATE_IDLE) {
        teach_.startTeach(motors_, linkC3_);
        state_ = STATE_TEACH_RECORD;
      }
      break;

    case CMD_TEACH_STOP:
      if (state_ == STATE_TEACH_RECORD) {
        teach_.stopTeach(motors_, linkC3_);
        state_ = STATE_IDLE;
      }
      break;

    case CMD_TEACH_SAVE_POINT:
      teach_.savePoint(servos_, linkC3_);
      break;

    case CMD_TEACH_SAVE_PROGRAM:
      teach_.saveProgram(cmd.programName);
      break;

    case CMD_TEACH_CLEAR:
      teach_.clearWaypoints();
      break;

    case CMD_PLAY_PROGRAM:
      if (state_ == STATE_IDLE) {
        teach_.loadProgram(cmd.programName);
        if (teach_.startPlayback(motors_, servos_)) state_ = STATE_TEACH_PLAY;
      }
      break;

    case CMD_PLAY_PAUSE:
      if (teach_.isPlaying()) {
        if (teach_.isPaused()) teach_.resumePlayback(motors_, servos_);
        else teach_.pausePlayback(motors_);
      }
      break;

    case CMD_PLAY_STOP:
      teach_.stopPlayback(motors_);
      if (state_ == STATE_TEACH_PLAY) state_ = STATE_IDLE;
      break;

    case CMD_PLAY_LOOP_TOGGLE:
      teach_.setLoop(!teach_.loopEnabled());
      break;

    case CMD_ESTOP:
      motors_.requestEStop();
      teach_.stopPlayback(motors_);
      errorAxisMask_ = 0xFF;
      state_ = STATE_ERROR;
      break;

    case CMD_CLEAR_ERROR:
      motors_.clearEStop();
      errorAxisMask_ = 0;
      state_ = STATE_IDLE;
      break;

    default:
      break;
  }
}

void StateMachine::checkJogWatchdog() {
  if (motors_.isAnyJogging() && (millis() - lastAnyJogCmdMs_ > JOG_KEEPALIVE_TIMEOUT_MS)) {
    motors_.stopAllJog();
  }
}

void StateMachine::checkRuntimeFaults() {
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    if (motors_.consumeStallFault(i)) {
      errorAxisMask_ |= (1 << i);
      motors_.forceStopAll();
      teach_.stopPlayback(motors_);
      state_ = STATE_ERROR;
    }
  }
}

void StateMachine::updateStateTransitions() {
  if (state_ == STATE_JOG && !motors_.isAnyJogging()) {
    state_ = STATE_IDLE;
  }
  if (state_ == STATE_TEACH_PLAY && !teach_.isPlaying()) {
    state_ = STATE_IDLE;
  }
}

void StateMachine::sendStatus() {
  StatusMsg msg;
  msg.seq = millis();
  msg.state = state_;
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    msg.axisPos[i] = motors_.getPositionUnits(i);
    msg.sgResult[i] = motors_.getSGResult(i);
  }
  msg.axisPos[AXIS_B] = servos_.getDeg(0);
  msg.axisPos[AXIS_C] = servos_.getDeg(1);
  msg.axisFault = errorAxisMask_;
  msg.waypointCount = teach_.waypointCount();
  msg.playIndex = teach_.playIndex();
  msg.playTotal = teach_.playTotal();
  msg.loopEnabled = teach_.loopEnabled() ? 1 : 0;
  linkHMI_.sendStatus(msg);

  // Cùng dữ liệu, đóng gói JSON gửi cho web qua WebSocket.
  static const char *const kStateNames[] = {"IDLE", "HOMING", "JOG", "TEACH_RECORD", "TEACH_PLAY", "ERROR"};
  JsonDocument doc;
  doc["type"] = "status";
  doc["state"] = kStateNames[state_ <= STATE_ERROR ? state_ : 0];
  doc["moving"] = motors_.isAnyMoving();
  doc["fault"] = errorAxisMask_;
  JsonArray p = doc["p"].to<JsonArray>();
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) p.add(motors_.getPositionUnits(i));
  p.add(servos_.getDeg(0));
  p.add(servos_.getDeg(1));
  String json;
  serializeJson(doc, json);
  linkWeb_.broadcastStatus(json);
}

// Đếm & sao chép tối đa `maxCount` điểm (mỗi điểm AXIS_COUNT số) từ mảng
// JSON `pts` (dạng [[j1..j6], [j1..j6], ...]) vào `out`. Dùng chung cho
// RUN_CYCLE và RUN_QUEUE.
static uint8_t collectPoints(JsonArrayConst pts, float out[][AXIS_COUNT], uint8_t maxCount) {
  uint8_t n = 0;
  for (JsonArrayConst pose : pts) {
    if (n >= maxCount) break;
    for (uint8_t i = 0; i < AXIS_COUNT && i < pose.size(); i++) out[n][i] = pose[i].as<float>();
    n++;
  }
  return n;
}

void StateMachine::handleWebCommand(const String &json) {
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) return;

  const char *cmd = doc["cmd"] | "";

  if (strcmp(cmd, "J") == 0 || strcmp(cmd, "START") == 0) {
    if (strcmp(cmd, "START") == 0) {
      motors_.clearEStop();
      errorAxisMask_ = 0;
      if (state_ == STATE_ERROR) state_ = STATE_IDLE;
    }
    JsonArrayConst p = doc["p"];
    if (p.size() >= AXIS_COUNT && state_ != STATE_TEACH_RECORD && state_ != STATE_TEACH_PLAY) {
      for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) motors_.moveTo(i, p[i].as<float>());
      servos_.setDeg(0, p[AXIS_B].as<float>());
      servos_.setDeg(1, p[AXIS_C].as<float>());
    }

  } else if (strcmp(cmd, "HOME") == 0) {
    if (state_ != STATE_TEACH_RECORD && state_ != STATE_TEACH_PLAY) {
      state_ = STATE_HOMING;
      bool allOk = true;
      for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
        if (!motors_.homeAxis(i)) {
          allOk = false;
          errorAxisMask_ |= (1 << i);
        }
      }
      state_ = allOk ? STATE_IDLE : STATE_ERROR;
    }

  } else if (strcmp(cmd, "ESTOP") == 0) {
    motors_.requestEStop();
    teach_.stopPlayback(motors_);
    errorAxisMask_ = 0xFF;
    state_ = STATE_ERROR;

  } else if (strcmp(cmd, "PING") == 0) {
    sendStatus();

  } else if (strcmp(cmd, "RUN_CYCLE") == 0 || strcmp(cmd, "RUN_QUEUE") == 0) {
    if (state_ == STATE_IDLE) {
      uint8_t count = 0;
      if (strcmp(cmd, "RUN_CYCLE") == 0) {
        count = collectPoints(doc["pts"], webPointsBuf_, MAX_WAYPOINTS);
      } else {
        for (JsonObjectConst item : doc["items"].as<JsonArrayConst>()) {
          if (count >= MAX_WAYPOINTS) break;
          count += collectPoints(item["pts"], &webPointsBuf_[count], (uint8_t)(MAX_WAYPOINTS - count));
        }
      }
      if (count > 0) {
        teach_.loadAdHocSequence(webPointsBuf_, count);
        if (teach_.startPlayback(motors_, servos_)) state_ = STATE_TEACH_PLAY;
      }
    }
  }
}
