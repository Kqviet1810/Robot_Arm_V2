#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "Config.h"

// Điều khiển 2 servo SG90: B (xoay kẹp), C (kẹp).
class ServoControl {
 public:
  void begin() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    servoB_.setPeriodHertz(50);
    servoC_.setPeriodHertz(50);
    servoB_.attach(SERVO_B_PIN, 500, 2400);
    servoC_.attach(SERVO_C_PIN, 500, 2400);
    setDeg(0, SERVO_B_DEFAULT_DEG);
    setDeg(1, SERVO_C_DEFAULT_DEG);
  }

  // servoIdx: 0 = B, 1 = C
  void setDeg(uint8_t servoIdx, float deg) {
    if (deg < SERVO_MIN_DEG) deg = SERVO_MIN_DEG;
    if (deg > SERVO_MAX_DEG) deg = SERVO_MAX_DEG;
    currentDeg_[servoIdx] = deg;
    (servoIdx == 0 ? servoB_ : servoC_).write((int)deg);
  }

  float getDeg(uint8_t servoIdx) const { return currentDeg_[servoIdx]; }

 private:
  Servo servoB_;
  Servo servoC_;
  float currentDeg_[2] = {(float)SERVO_B_DEFAULT_DEG, (float)SERVO_C_DEFAULT_DEG};
};
