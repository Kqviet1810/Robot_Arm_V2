#pragma once
#include <RobotArmProtocol.h>

// ============================================================================
// Cấu hình phần cứng cho ESP32 Wroom (Main Controller)
// Toàn bộ hằng số "cần chỉnh theo phần cứng thật" gom ở đây, không rải
// trong logic điều khiển.
// ============================================================================

// --- Chân STEP/DIR 4 trục (thứ tự khớp AxisId: X,Y,Z,A) ---
static const int STEP_PIN[AXIS_STEPPER_COUNT] = {14, 26, 18, 21};
static const int DIR_PIN[AXIS_STEPPER_COUNT] = {27, 25, 4, 19};

// --- Chân DIAG TMC2209 (StallGuard, dùng cho homing sensorless) ---
static const int DIAG_PIN[AXIS_STEPPER_COUNT] = {35, 34, 36, 39};

// --- UART1: bus TMC2209 multi-drop (4 driver dùng chung TX/RX, phân biệt
// bằng địa chỉ đặt qua điện trở tại chân MS1/MS2 của từng driver) ---
static const int PIN_TMC_UART_TX = 23;
static const int PIN_TMC_UART_RX = 22;
static const uint32_t TMC_UART_BAUD = 115200;
// Địa chỉ UART từng driver (đặt bằng MS1/MS2), khớp thứ tự X,Y,Z,A.
static const uint8_t TMC_ADDR[AXIS_STEPPER_COUNT] = {0, 1, 2, 3};
// Điện trở sense trên module driver — CẦN xác nhận lại theo module TMC2209
// thực tế đang dùng (phổ biến 0.11R trên board BTT/Fysetc).
static constexpr float TMC_R_SENSE = 0.11f;
// Dòng điện cấp cho động cơ (mA, RMS) — giá trị khởi điểm an toàn, cần đo
// dòng định mức động cơ thật rồi chỉnh lại (không vượt dòng cho phép driver/motor).
static const uint16_t TMC_RMS_CURRENT_MA = 800;
static const uint16_t TMC_MICROSTEPS = 16;

// --- UART2: liên kết Wroom <-> ESP32 C3 ---
static const int PIN_C3_UART_TX = 17;
static const int PIN_C3_UART_RX = 16;
static const uint32_t C3_UART_BAUD = 115200;

// --- Servo SG90 ---
static const int SERVO_B_PIN = 32;  // xoay kẹp
static const int SERVO_C_PIN = 33;  // kẹp
static const uint8_t SERVO_B_DEFAULT_DEG = 90;
static const uint8_t SERVO_C_DEFAULT_DEG = 90;
static const uint8_t SERVO_MIN_DEG = 0;
static const uint8_t SERVO_MAX_DEG = 180;

// --- Cơ khí: tỉ số truyền & quy đổi bước ---
// X, Y, Z: puly chủ động GT2-20T -> puly bị động GT2-90T (tỉ số 4.5:1).
//   Z = xoay toa chính, X/Y = xoay cánh tay.
// A: trục ray, puly GT2-20T (bước răng 2mm) kéo dây đai dọc ray dài 450mm
//   -> 40mm di chuyển mỗi vòng quay động cơ.
static const float MOTOR_STEPS_PER_REV = 200.0f;  // động cơ bước 1.8°/step
static const float PULLEY_RATIO_XYZ = 90.0f / 20.0f;  // 4.5:1
static const float GT2_PITCH_MM = 2.0f;
static const float GT2_DRIVE_TEETH_A = 20.0f;
static const float RAIL_LENGTH_MM = 450.0f;

inline float stepsPerDegXYZ() {
  return (MOTOR_STEPS_PER_REV * TMC_MICROSTEPS * PULLEY_RATIO_XYZ) / 360.0f;
}
inline float stepsPerMmA() {
  return (MOTOR_STEPS_PER_REV * TMC_MICROSTEPS) / (GT2_PITCH_MM * GT2_DRIVE_TEETH_A);
}

// Quy đổi độ lệch góc TRỤC ĐỘNG CƠ (đo trực tiếp bằng AS5600 ở đuôi động cơ,
// đã unwrap) sang đơn vị đầu ra của khớp (độ với X/Y/Z, mm với A). Dùng khi
// đồng bộ lại vị trí sau khi thả trơn (freewheel) trong chế độ cầm tay chỉ việc.
inline float motorDegToOutputUnits(uint8_t axisIdx, float motorDeg) {
  if (axisIdx < 3) return motorDeg / PULLEY_RATIO_XYZ;
  return motorDeg / 360.0f * (GT2_PITCH_MM * GT2_DRIVE_TEETH_A);
}

// --- Giới hạn hành trình mềm (soft limit), đơn vị độ (X/Y/Z) hoặc mm (A) ---
// TODO: chỉnh lại theo giới hạn cơ khí thật của từng khớp sau khi lắp ráp.
static const float AXIS_SOFT_MIN[AXIS_STEPPER_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f};
static const float AXIS_SOFT_MAX[AXIS_STEPPER_COUNT] = {180.0f, 180.0f, 360.0f, RAIL_LENGTH_MM};

// --- StallGuard (homing sensorless) ---
// Ngưỡng SGTHRS do người dùng cung cấp: X/Y/Z=30, A=40.
static const uint8_t SGTHRS[AXIS_STEPPER_COUNT] = {30, 30, 30, 40};
// Chỉ kích hoạt StallGuard khi tốc độ bước dưới ngưỡng này (TCOOLTHRS),
// nên đặt bằng hoặc hơi cao hơn tốc độ homing.
static const uint32_t TCOOLTHRS_VALUE = 400;
// Hướng homing: -1 = về phía giá trị nhỏ hơn, +1 = về phía giá trị lớn hơn.
// X,Y,Z: Negative Limit theo tài liệu gốc -> -1. A: Positive Limit -> +1.
static const int8_t HOMING_DIR[AXIS_STEPPER_COUNT] = {-1, -1, -1, +1};
static const float HOMING_SPEED_DEG_S = 15.0f;   // tốc độ dò home, X/Y/Z (độ/giây)
static const float HOMING_SPEED_MM_S = 15.0f;    // tốc độ dò home, A (mm/giây)
static const float HOMING_BACKOFF_DEG = 3.0f;
static const float HOMING_BACKOFF_MM = 5.0f;
static const uint32_t HOMING_TIMEOUT_MS = 15000;

// --- Tốc độ/gia tốc vận hành bình thường (jog, playback) ---
static const float MAX_SPEED_DEG_S = 60.0f;
static const float MAX_SPEED_MM_S = 40.0f;
static const float ACCEL_DEG_S2 = 120.0f;
static const float ACCEL_MM_S2 = 80.0f;

// --- Teach & Playback ---
// 200 điểm đủ cho teach thủ công lẫn RUN_CYCLE/RUN_QUEUE gộp nhiều chu
// trình từ web (chi phí RAM không đáng kể: 200 * 6 float = 4.8KB).
static const uint8_t MAX_WAYPOINTS = 200;
static const float PLAYBACK_POSITION_TOLERANCE_DEG = 1.0f;

// --- An toàn ---
// Nếu không nhận CommandMsg mới trong lúc jog quá thời gian này -> tự dừng.
static const uint32_t JOG_KEEPALIVE_TIMEOUT_MS = 300;
static const uint32_t STATUS_SEND_INTERVAL_MS = 100;

// --- WiFi SoftAP + WebSocket (kết nối trực tiếp từ web dashboard) ---
// Đổi mật khẩu trước khi dùng thật nếu cần bảo mật cao hơn (>= 8 ký tự).
static const char *const WIFI_AP_SSID = "RobotArmV2";
static const char *const WIFI_AP_PASSWORD = "robotarm123";
// Kênh WiFi cố định để ESP-NOW với C3/CYD không bị lệch kênh khi Wroom
// chuyển sang chạy AP (thay vì STA mặc định) — C3/CYD phải pin cùng kênh
// này (xem WIFI_AP_CHANNEL trong Config.h của 2 board đó).
static const uint8_t WIFI_AP_CHANNEL = 6;
