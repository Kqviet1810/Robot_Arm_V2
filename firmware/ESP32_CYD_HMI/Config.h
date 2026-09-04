#pragma once
#include <RobotArmProtocol.h>

// ============================================================================
// Cấu hình phần cứng cho ESP32 CYD (ESP32-2432S028, 2.8" ILI9341 + XPT2046)
// Dùng pinout chuẩn cộng đồng cho board này.
// ============================================================================

// --- TFT (ILI9341, SPI dùng chung với touch) ---
static const int TFT_PIN_MOSI = 13;
static const int TFT_PIN_MISO = 12;
static const int TFT_PIN_SCLK = 14;
static const int TFT_PIN_CS = 15;
static const int TFT_PIN_DC = 2;
static const int TFT_PIN_RST = -1;  // nối EN, không dùng chân riêng
static const int TFT_PIN_BL = 21;

// --- Touch (XPT2046, dùng chung SPI với TFT, CS riêng) ---
static const int TOUCH_PIN_CS = 33;
static const int TOUCH_PIN_IRQ = 36;

static const int SCREEN_WIDTH = 320;
static const int SCREEN_HEIGHT = 240;

// Hiệu chỉnh vùng đọc thô của cảm ứng điện trở XPT2046 — GIÁ TRỊ MẶC ĐỊNH,
// CẦN đo lại trên board thật (in ra giá trị thô khi chạm 4 góc màn hình rồi
// cập nhật lại 4 hằng số dưới đây) vì sai số khác nhau giữa các lô sản xuất.
static const int TOUCH_RAW_X_MIN = 200;
static const int TOUCH_RAW_X_MAX = 3800;
static const int TOUCH_RAW_Y_MIN = 200;
static const int TOUCH_RAW_Y_MAX = 3800;

// --- ESP-NOW ---
static const uint32_t COMMAND_KEEPALIVE_INTERVAL_MS = 150;  // gửi lại JOG_START khi giữ nút
static const uint32_t LINK_FRESH_TIMEOUT_MS = 800;

// ESP32 Wroom chạy WiFi SoftAP (phục vụ web dashboard) trên kênh cố định —
// CYD phải pin đúng kênh này thì ESP-NOW mới nhận được (xem WIFI_AP_CHANNEL
// trong ESP32_Wroom_Main/Config.h, PHẢI khớp giá trị).
static const uint8_t WIFI_AP_CHANNEL = 6;

// --- Jog ---
static const uint8_t JOG_SPEED_SLOW_PCT = 30;
static const uint8_t JOG_SPEED_FAST_PCT = 100;
static const float SERVO_STEP_DEG = 5.0f;

// --- Tên chương trình teach & playback (slot cố định, tránh cần bàn phím ảo) ---
static const uint8_t PROGRAM_SLOT_COUNT = 5;
static const char *const PROGRAM_SLOT_NAMES[PROGRAM_SLOT_COUNT] = {
    "CT1", "CT2", "CT3", "CT4", "CT5"};
