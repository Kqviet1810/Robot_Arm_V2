#pragma once
#include <Arduino.h>
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include <RobotArmProtocol.h>
#include "Config.h"

// Điều khiển 4 trục stepper (X,Y,Z,A) qua TMC2209 (cấu hình bằng UART,
// multi-drop 1 bus chung) + sinh xung STEP/DIR bằng FastAccelStepper.
//
// Homing dùng StallGuard (sensorless, không công tắc hành trình cơ khí):
// driver báo "stall" qua chân DIAG khi mô-men cản vượt ngưỡng SGTHRS, xử lý
// bằng ngắt ngoài (interrupt) trên từng chân DIAG.
//
// Lưu ý dây UART: 4 module TMC2209 dùng chung 1 bus PDN_UART (half-duplex).
// Cách đấu phổ biến: nối chân TX của ESP32 qua điện trở ~1k tới PDN_UART,
// đồng thời nối thẳng chân RX của ESP32 vào cùng PDN_UART -> khi ESP32 gửi,
// tự nhận lại byte mình gửi (echo); thư viện TMCStepper tự xử lý phần echo
// này khi đọc phản hồi từ driver.
class MotorControl {
 public:
  void begin();
  void update();  // gọi mỗi vòng loop(): kiểm tra soft-limit khi đang jog, xử lý fault runtime

  void startJog(uint8_t axisIdx, int8_t dir, uint8_t speedPct);
  void stopJog(uint8_t axisIdx);
  void stopAllJog();
  void forceStopAll();
  bool isJogging(uint8_t axisIdx) const { return axisIdx < AXIS_STEPPER_COUNT && jogging_[axisIdx]; }
  bool isAnyJogging() const {
    for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) if (jogging_[i]) return true;
    return false;
  }

  // Homing 1 trục, BLOCKING (an toàn cho phase hiện tại vì không cần chạy
  // song song nhiều trục lúc home). Trả về false nếu timeout hoặc bị hủy
  // bởi E-Stop.
  bool homeAxis(uint8_t axisIdx);

  // Di chuyển tới vị trí đích (độ với X/Y/Z, mm với A), tự kẹp trong soft
  // limit. Không chặn (non-blocking) — dùng isMoving()/waitUntilIdle() để
  // theo dõi.
  void moveTo(uint8_t axisIdx, float targetUnits);
  bool isMoving(uint8_t axisIdx) const;
  // true nếu bất kỳ trục stepper nào đang di chuyển — dùng cho status JSON
  // gửi web (độc lập với RobotState/jogging_ để không đụng logic ESP-NOW).
  bool isAnyMoving() const {
    for (uint8_t i = 0; i < AXIS_STEPPER_COUNT; i++) {
      if (steppers_[i] && steppers_[i]->isRunning()) return true;
    }
    return false;
  }
  // Chờ tới khi trục dừng hoặc hết timeout/E-Stop. Trả về true nếu dừng vì
  // đã tới đích (không phải timeout/E-Stop).
  bool waitUntilIdle(uint8_t axisIdx, uint32_t timeoutMs);

  // Thả trơn (freewheel) để cầm tay chỉ việc: tắt dòng giữ động cơ qua UART
  // (TOFF=0) thay vì dùng chân EN vật lý (board hiện không có chân EN riêng).
  void setFreewheel(uint8_t axisIdx, bool enable);
  void setFreewheelAll(bool enable);

  float getPositionUnits(uint8_t axisIdx) const;
  // Đặt lại vị trí hiện tại (dùng sau homing, hoặc sau khi thả trơn để
  // đồng bộ lại theo góc AS5600 thật trước khi playback).
  void setCurrentPositionUnits(uint8_t axisIdx, float units);

  int16_t getSGResult(uint8_t axisIdx);
  // true nếu trục vừa stall ngoài lúc homing (lỗi runtime) — tự xoá cờ khi đọc.
  bool consumeStallFault(uint8_t axisIdx);

  void requestEStop() { eStop_ = true; forceStopAll(); }
  void clearEStop() { eStop_ = false; }
  bool isEStopped() const { return eStop_; }

 private:
  HardwareSerial tmcSerial_{1};
  TMC2209Stepper driverX_{&tmcSerial_, TMC_R_SENSE, TMC_ADDR[0]};
  TMC2209Stepper driverY_{&tmcSerial_, TMC_R_SENSE, TMC_ADDR[1]};
  TMC2209Stepper driverZ_{&tmcSerial_, TMC_R_SENSE, TMC_ADDR[2]};
  TMC2209Stepper driverA_{&tmcSerial_, TMC_R_SENSE, TMC_ADDR[3]};
  TMC2209Stepper *drivers_[AXIS_STEPPER_COUNT];

  FastAccelStepperEngine engine_;
  FastAccelStepper *steppers_[AXIS_STEPPER_COUNT] = {nullptr, nullptr, nullptr, nullptr};

  volatile bool diagFlag_[AXIS_STEPPER_COUNT] = {false, false, false, false};
  bool jogging_[AXIS_STEPPER_COUNT] = {false, false, false, false};
  bool freewheel_[AXIS_STEPPER_COUNT] = {false, false, false, false};
  bool eStop_ = false;

  static MotorControl *self_;
  static void IRAM_ATTR diagIsr0();
  static void IRAM_ATTR diagIsr1();
  static void IRAM_ATTR diagIsr2();
  static void IRAM_ATTR diagIsr3();

  void configureDriver(uint8_t axisIdx);
  float stepsPerUnit(uint8_t axisIdx) const;
  long unitsToSteps(uint8_t axisIdx, float units) const;
  float stepsToUnits(uint8_t axisIdx, long steps) const;
  float clampToSoftLimit(uint8_t axisIdx, float units) const;
};
