// ESP32_CYD_HMI — Robot Arm V2
// Board: ESP32 CYD (ESP32-2432S028, 2.8" ILI9341 + XPT2046 touch)
// Vai trò: màn hình điều khiển — hiển thị trạng thái, gửi lệnh jog/home/
// teach/playback/E-Stop tới ESP32 Wroom, hiển thị dữ liệu giám sát từ
// ESP32 C3 — toàn bộ qua ESP-NOW.
//
// Yêu cầu thư viện: RobotArmProtocol (nội bộ), TFT_eSPI, lvgl (v8.x, CẦN có
// lv_conf.h — xem firmware/README.md), XPT2046_Touchscreen.
// LƯU Ý: lv_conf.h phải đặt LV_COLOR_DEPTH thành 16 để khớp hàm dispFlush().

#include "TFT_eSPI_Setup.h"  // PHẢI include trước TFT_eSPI.h
#include <TFT_eSPI.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include <RobotArmProtocol.h>
#include "Config.h"
#include "LinkWroom.h"
#include "UI_Common.h"

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(TOUCH_PIN_CS, TOUCH_PIN_IRQ);
LinkWroom linkWroom;

static lv_disp_draw_buf_t sDrawBuf;
static lv_color_t sBuf1[SCREEN_WIDTH * 10];
static uint32_t sLastUiTickMs = 0;

static void dispFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
  uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

static void touchpadRead(lv_indev_drv_t *indev, lv_indev_data_t *data) {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int x = constrain(map(p.x, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, SCREEN_WIDTH - 1), 0, SCREEN_WIDTH - 1);
    int y = constrain(map(p.y, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, SCREEN_HEIGHT - 1), 0, SCREEN_HEIGHT - 1);
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(TFT_PIN_BL, OUTPUT);
  digitalWrite(TFT_PIN_BL, HIGH);

  tft.begin();
  tft.setRotation(1);

  ts.begin();
  ts.setRotation(1);

  lv_init();
  lv_disp_draw_buf_init(&sDrawBuf, sBuf1, nullptr, SCREEN_WIDTH * 10);

  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = SCREEN_WIDTH;
  dispDrv.ver_res = SCREEN_HEIGHT;
  dispDrv.flush_cb = dispFlush;
  dispDrv.draw_buf = &sDrawBuf;
  lv_disp_drv_register(&dispDrv);

  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = touchpadRead;
  lv_indev_drv_register(&indevDrv);

  if (!linkWroom.begin()) {
    Serial.println("[CYD] Loi khoi tao ESP-NOW");
  }
  uiInit(&linkWroom);

  Serial.println("[CYD] HMI san sang");
}

void loop() {
  lv_timer_handler();
  if (millis() - sLastUiTickMs > 100) {
    sLastUiTickMs = millis();
    uiTick();
  }
  delay(5);
}
