#pragma once
#include <RobotArmProtocol.h>

// ============================================================================
// Cấu hình phần cứng cho ESP32 C3 Super Mini (board đọc cảm biến AS5600)
// ============================================================================

// --- I2C -> TCA9548A -> 4x AS5600 ---
static const int PIN_I2C_SDA = 8;
static const int PIN_I2C_SCL = 9;
static const uint32_t I2C_CLOCK_HZ = 400000;

static const uint8_t TCA9548A_ADDR = 0x70;
static const uint8_t AS5600_ADDR = 0x36;

// Channel TCA9548A 0..3 tương ứng trục X,Y,Z,A (đúng thứ tự AxisId trong
// RobotArmCommon.h: AXIS_X=0, AXIS_Y=1, AXIS_Z=2, AXIS_A=3).
static const uint8_t AS5600_CHANNEL[AXIS_STEPPER_COUNT] = {0, 1, 2, 3};

// --- UART -> ESP32 Wroom (Main) ---
static const int PIN_UART_TX = 6;
static const int PIN_UART_RX = 7;
static const uint32_t UART_BAUD = 115200;
static const uint32_t ANGLE_REPORT_INTERVAL_MS = 20;   // ~50Hz gửi Wroom

// --- ESP-NOW -> CYD (telemetry giám sát) ---
static const uint32_t TELEMETRY_INTERVAL_MS = 100;     // ~10Hz gửi CYD

// ESP32 Wroom giờ chạy WiFi SoftAP (phục vụ web dashboard) thay vì STA, trên
// kênh cố định — C3 phải pin đúng kênh này thì ESP-NOW mới nhận được
// (xem WIFI_AP_CHANNEL trong ESP32_Wroom_Main/Config.h, PHẢI khớp giá trị).
static const uint8_t WIFI_AP_CHANNEL = 6;
