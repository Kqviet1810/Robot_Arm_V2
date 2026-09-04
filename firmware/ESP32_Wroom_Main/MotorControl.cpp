#include "MotorControl.h"
#include <math.h>

MotorControl *MotorControl::self_ = nullptr;

void IRAM_ATTR MotorControl::diagIsr0() { if (self_) self_->diagFlag_[0] = true; }
void IRAM_ATTR MotorControl::diagIsr1() { if (self_) self_->diagFlag_[1] = true; }
void IRAM_ATTR MotorControl::diagIsr2() { if (self_) self_->diagFlag_[2] = true; }
void IRAM_ATTR MotorControl::diagIsr3() { if (self_) self_->diagFlag_[3] = true; }

void MotorControl::begin() {
  self_ = this;
  drivers_[0] = &driverX_;
  drivers_[1] = &driverY_;
  drivers_[2] = &driverZ_;
  drivers_[3] = &driverA_;

  tmcSerial_.begin(TMC_UART_BAUD, SERIAL_8N1, PIN_TMC_UART_RX, PIN_TMC_UART_TX);
  delay(50);
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) configureDriver(i);

  engine_.init();
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    steppers_[i] = engine_.stepperConnectToPin(STEP_PIN[i]);
    if (!steppers_[i]) continue;
    steppers_[i]->setDirectionPin(DIR_PIN[i]);
    steppers_[i]->setAutoEnable(false);
    float maxSpeedUnitsS = (i < 3) ? MAX_SPEED_DEG_S : MAX_SPEED_MM_S;
    float accelUnitsS2 = (i < 3) ? ACCEL_DEG_S2 : ACCEL_MM_S2;
    steppers_[i]->setSpeedInHz((uint32_t)(maxSpeedUnitsS * stepsPerUnit(i)));
    steppers_[i]->setAcceleration((int32_t)(accelUnitsS2 * stepsPerUnit(i)));
    steppers_[i]->setCurrentPosition(0);

    pinMode(DIAG_PIN[i], INPUT);
  }

  attachInterrupt(digitalPinToInterrupt(DIAG_PIN[0]), diagIsr0, RISING);
  attachInterrupt(digitalPinToInterrupt(DIAG_PIN[1]), diagIsr1, RISING);
  attachInterrupt(digitalPinToInterrupt(DIAG_PIN[2]), diagIsr2, RISING);
  attachInterrupt(digitalPinToInterrupt(DIAG_PIN[3]), diagIsr3, RISING);
}

void MotorControl::configureDriver(uint8_t axisIdx) {
  TMC2209Stepper *drv = drivers_[axisIdx];
  drv->begin();
  drv->toff(4);
  drv->rms_current(TMC_RMS_CURRENT_MA);
  drv->microsteps(TMC_MICROSTEPS);
  drv->pwm_autoscale(true);
  drv->en_spreadCycle(false);  // StealthChop (êm), StallGuard vẫn hoạt động qua TCOOLTHRS
  drv->TCOOLTHRS(TCOOLTHRS_VALUE);
  drv->SGTHRS(SGTHRS[axisIdx]);
  freewheel_[axisIdx] = false;
}

void MotorControl::update() {
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    if (jogging_[i]) {
      float pos = getPositionUnits(i);
      if (pos <= AXIS_SOFT_MIN[i] || pos >= AXIS_SOFT_MAX[i]) {
        stopJog(i);
      }
    }
  }
}

void MotorControl::startJog(uint8_t axisIdx, int8_t dir, uint8_t speedPct) {
  if (axisIdx >= AXIS_STEPPER_COUNT || !steppers_[axisIdx] || eStop_ || freewheel_[axisIdx]) return;
  if (speedPct < 1) speedPct = 1;
  if (speedPct > 100) speedPct = 100;

  float maxSpeedUnitsS = (axisIdx < 3) ? MAX_SPEED_DEG_S : MAX_SPEED_MM_S;
  float speedUnitsS = maxSpeedUnitsS * (speedPct / 100.0f);
  steppers_[axisIdx]->setSpeedInHz((uint32_t)(speedUnitsS * stepsPerUnit(axisIdx)));

  jogging_[axisIdx] = true;
  if (dir >= 0) {
    steppers_[axisIdx]->runForward();
  } else {
    steppers_[axisIdx]->runBackward();
  }
}

void MotorControl::stopJog(uint8_t axisIdx) {
  if (axisIdx >= AXIS_STEPPER_COUNT || !steppers_[axisIdx]) return;
  jogging_[axisIdx] = false;
  steppers_[axisIdx]->stopMove();
}

void MotorControl::stopAllJog() {
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) stopJog(i);
}

void MotorControl::forceStopAll() {
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
    jogging_[i] = false;
    if (steppers_[i]) steppers_[i]->forceStopAndNewPosition(steppers_[i]->getCurrentPosition());
  }
}

bool MotorControl::homeAxis(uint8_t axisIdx) {
  if (axisIdx >= AXIS_STEPPER_COUNT || !steppers_[axisIdx] || eStop_) return false;

  diagFlag_[axisIdx] = false;
  float homingSpeedUnitsS = (axisIdx < 3) ? HOMING_SPEED_DEG_S : HOMING_SPEED_MM_S;
  steppers_[axisIdx]->setSpeedInHz((uint32_t)(homingSpeedUnitsS * stepsPerUnit(axisIdx)));

  if (HOMING_DIR[axisIdx] >= 0) {
    steppers_[axisIdx]->runForward();
  } else {
    steppers_[axisIdx]->runBackward();
  }

  uint32_t startMs = millis();
  bool stalled = false;
  while (millis() - startMs < HOMING_TIMEOUT_MS) {
    if (eStop_) break;
    if (diagFlag_[axisIdx]) { stalled = true; break; }
    delay(2);
  }

  steppers_[axisIdx]->forceStopAndNewPosition(steppers_[axisIdx]->getCurrentPosition());
  diagFlag_[axisIdx] = false;

  // khôi phục tốc độ vận hành bình thường cho các lệnh sau
  float maxSpeedUnitsS = (axisIdx < 3) ? MAX_SPEED_DEG_S : MAX_SPEED_MM_S;
  steppers_[axisIdx]->setSpeedInHz((uint32_t)(maxSpeedUnitsS * stepsPerUnit(axisIdx)));

  if (!stalled) return false;  // timeout hoặc bị huỷ

  // lùi ra khỏi điểm stall một đoạn nhỏ theo hướng ngược lại
  float backoffUnits = (axisIdx < 3) ? HOMING_BACKOFF_DEG : HOMING_BACKOFF_MM;
  long backoffSteps = (long)(backoffUnits * stepsPerUnit(axisIdx));
  long target = steppers_[axisIdx]->getCurrentPosition() + (HOMING_DIR[axisIdx] >= 0 ? -backoffSteps : backoffSteps);
  steppers_[axisIdx]->moveTo(target);
  waitUntilIdle(axisIdx, 5000);

  steppers_[axisIdx]->setCurrentPosition(0);
  return true;
}

void MotorControl::moveTo(uint8_t axisIdx, float targetUnits) {
  if (axisIdx >= AXIS_STEPPER_COUNT || !steppers_[axisIdx] || eStop_ || freewheel_[axisIdx]) return;
  jogging_[axisIdx] = false;
  float clamped = clampToSoftLimit(axisIdx, targetUnits);
  steppers_[axisIdx]->moveTo(unitsToSteps(axisIdx, clamped));
}

bool MotorControl::isMoving(uint8_t axisIdx) const {
  if (axisIdx >= AXIS_STEPPER_COUNT || !steppers_[axisIdx]) return false;
  return steppers_[axisIdx]->isRunning();
}

bool MotorControl::waitUntilIdle(uint8_t axisIdx, uint32_t timeoutMs) {
  uint32_t startMs = millis();
  while (isMoving(axisIdx)) {
    if (eStop_) return false;
    if (millis() - startMs > timeoutMs) return false;
    delay(2);
  }
  return true;
}

void MotorControl::setFreewheel(uint8_t axisIdx, bool enable) {
  if (axisIdx >= AXIS_STEPPER_COUNT) return;
  freewheel_[axisIdx] = enable;
  drivers_[axisIdx]->toff(enable ? 0 : 4);
  if (enable) stopJog(axisIdx);
}

void MotorControl::setFreewheelAll(bool enable) {
  for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) setFreewheel(i, enable);
}

float MotorControl::getPositionUnits(uint8_t axisIdx) const {
  if (axisIdx >= AXIS_STEPPER_COUNT || !steppers_[axisIdx]) return 0.0f;
  return stepsToUnits(axisIdx, steppers_[axisIdx]->getCurrentPosition());
}

void MotorControl::setCurrentPositionUnits(uint8_t axisIdx, float units) {
  if (axisIdx >= AXIS_STEPPER_COUNT || !steppers_[axisIdx]) return;
  steppers_[axisIdx]->setCurrentPosition(unitsToSteps(axisIdx, units));
}

int16_t MotorControl::getSGResult(uint8_t axisIdx) {
  if (axisIdx >= AXIS_STEPPER_COUNT) return 0;
  return (int16_t)drivers_[axisIdx]->SG_RESULT();
}

bool MotorControl::consumeStallFault(uint8_t axisIdx) {
  if (axisIdx >= AXIS_STEPPER_COUNT) return false;
  // Chỉ coi là "fault" khi stall xảy ra ngoài lúc homing (homeAxis() đã tự
  // xoá cờ trước khi trả về) — nếu cờ còn set ở đây tức là stall bất ngờ
  // trong lúc jog/playback.
  bool wasSet = diagFlag_[axisIdx];
  diagFlag_[axisIdx] = false;
  return wasSet;
}

float MotorControl::stepsPerUnit(uint8_t axisIdx) const {
  return (axisIdx < 3) ? stepsPerDegXYZ() : stepsPerMmA();
}

long MotorControl::unitsToSteps(uint8_t axisIdx, float units) const {
  return (long)lroundf(units * stepsPerUnit(axisIdx));
}

float MotorControl::stepsToUnits(uint8_t axisIdx, long steps) const {
  return (float)steps / stepsPerUnit(axisIdx);
}

float MotorControl::clampToSoftLimit(uint8_t axisIdx, float units) const {
  if (units < AXIS_SOFT_MIN[axisIdx]) return AXIS_SOFT_MIN[axisIdx];
  if (units > AXIS_SOFT_MAX[axisIdx]) return AXIS_SOFT_MAX[axisIdx];
  return units;
}
