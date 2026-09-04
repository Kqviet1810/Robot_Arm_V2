#include "TeachPlayback.h"
#include <LittleFS.h>

void TeachPlayback::begin() {
  fsReady_ = LittleFS.begin(true);
  if (!fsReady_) {
    Serial.println("[Wroom] Loi mount LittleFS (luu/tai chuong trinh se khong hoat dong)");
    return;
  }
  if (!LittleFS.exists("/programs")) {
    LittleFS.mkdir("/programs");
  }
}

void TeachPlayback::startTeach(MotorControl &motors, LinkC3 &linkC3) {
  teaching_ = true;
  count_ = 0;
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    teachStartUnwrapped_[i] = linkC3.unwrappedMotorDeg(i);
    teachStartPosUnits_[i] = motors.getPositionUnits(i);
  }
  motors.setFreewheelAll(true);
}

void TeachPlayback::stopTeach(MotorControl &motors, LinkC3 &linkC3) {
  if (!teaching_) return;
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    motors.setCurrentPositionUnits(i, liveEstimatedPositionUnits(i, linkC3));
  }
  motors.setFreewheelAll(false);
  teaching_ = false;
}

bool TeachPlayback::savePoint(ServoControl &servos, LinkC3 &linkC3) {
  if (!teaching_ || count_ >= kMaxWaypoints) return false;
  Waypoint &wp = waypoints_[count_];
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    wp.axisPos[i] = liveEstimatedPositionUnits(i, linkC3);
  }
  wp.axisPos[AXIS_B] = servos.getDeg(0);
  wp.axisPos[AXIS_C] = servos.getDeg(1);
  count_++;
  return true;
}

float TeachPlayback::liveEstimatedPositionUnits(uint8_t axisIdx, LinkC3 &linkC3) const {
  float deltaMotorDeg = linkC3.unwrappedMotorDeg(axisIdx) - teachStartUnwrapped_[axisIdx];
  return teachStartPosUnits_[axisIdx] + motorDegToOutputUnits(axisIdx, deltaMotorDeg);
}

bool TeachPlayback::saveProgram(const char *name) {
  if (!fsReady_ || count_ == 0) return false;
  String path = String("/programs/") + name + ".bin";
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  f.write(count_);
  f.write((const uint8_t *)waypoints_, sizeof(Waypoint) * count_);
  f.close();
  return true;
}

bool TeachPlayback::loadProgram(const char *name) {
  if (!fsReady_) return false;
  String path = String("/programs/") + name + ".bin";
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  uint8_t n = 0;
  if (f.read(&n, 1) != 1 || n > kMaxWaypoints) {
    f.close();
    return false;
  }
  size_t expected = sizeof(Waypoint) * n;
  size_t got = f.read((uint8_t *)waypoints_, expected);
  f.close();
  if (got != expected) return false;
  count_ = n;
  return true;
}

void TeachPlayback::loadAdHocSequence(const float points[][AXIS_COUNT], uint8_t count) {
  if (count > kMaxWaypoints) count = kMaxWaypoints;
  for (uint8_t i = 0; i < count; i++) {
    memcpy(waypoints_[i].axisPos, points[i], sizeof(waypoints_[i].axisPos));
  }
  count_ = count;
}

bool TeachPlayback::startPlayback(MotorControl &motors, ServoControl &servos) {
  if (teaching_ || count_ == 0) return false;
  playing_ = true;
  paused_ = false;
  playIdx_ = 0;
  commandWaypoint(playIdx_, motors, servos);
  return true;
}

void TeachPlayback::pausePlayback(MotorControl &motors) {
  if (!playing_) return;
  paused_ = true;
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) motors.stopJog(i);
}

void TeachPlayback::resumePlayback(MotorControl &motors, ServoControl &servos) {
  if (!playing_ || !paused_) return;
  paused_ = false;
  commandWaypoint(playIdx_, motors, servos);
}

void TeachPlayback::stopPlayback(MotorControl &motors) {
  playing_ = false;
  paused_ = false;
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) motors.stopJog(i);
}

void TeachPlayback::commandWaypoint(uint8_t idx, MotorControl &motors, ServoControl &servos) {
  Waypoint &wp = waypoints_[idx];
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) motors.moveTo(i, wp.axisPos[i]);
  servos.setDeg(0, wp.axisPos[AXIS_B]);
  servos.setDeg(1, wp.axisPos[AXIS_C]);
}

void TeachPlayback::update(MotorControl &motors, ServoControl &servos) {
  if (!playing_ || paused_) return;

  bool anyMoving = false;
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    if (motors.isMoving(i)) { anyMoving = true; break; }
  }
  if (anyMoving) return;

  playIdx_++;
  if (playIdx_ >= count_) {
    if (loop_) {
      playIdx_ = 0;
      commandWaypoint(playIdx_, motors, servos);
    } else {
      playing_ = false;
      playIdx_ = 0;
    }
  } else {
    commandWaypoint(playIdx_, motors, servos);
  }
}
