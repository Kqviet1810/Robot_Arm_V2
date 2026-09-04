#pragma once
// Cấu hình TFT_eSPI cho ESP32-2432S028 (CYD) — nạp trực tiếp trong sketch
// bằng USER_SETUP_LOADED thay vì phải sửa file User_Setup.h trong thư mục
// thư viện (tránh mất cấu hình khi thư viện được cập nhật).
// Include file này TRƯỚC <TFT_eSPI.h>.

#define USER_SETUP_LOADED 1

#define ILI9341_DRIVER

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1  // nối EN

#define TOUCH_CS 33  // XPT2046 dùng chung SPI, chỉ khác chân CS

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
