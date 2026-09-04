#include "UI_Jog.h"
#include "UI_Common.h"
#include "Config.h"
#include <RobotArmProtocol.h>

static const char *kAxisNames[AXIS_COUNT] = {"X", "Y", "Z", "A", "B", "C"};

static lv_obj_t *sValueLabel[AXIS_COUNT];
static lv_obj_t *sSpeedBtnLabel;
static uint8_t sSpeedPct = JOG_SPEED_SLOW_PCT;

struct JogBtnCtx {
  uint8_t axis;
  int8_t dir;
};
static JogBtnCtx sJogCtx[AXIS_STEPPER_COUNT * 2];
static uint32_t sLastJogSendMs[AXIS_STEPPER_COUNT] = {0, 0, 0, 0};

static void sendJogStart(uint8_t axis, int8_t dir) {
  CommandMsg cmd{};
  cmd.cmd = CMD_JOG_START;
  cmd.axis = axis;
  cmd.dir = dir;
  cmd.speedPct = sSpeedPct;
  uiSendCommand(cmd);
}

static void sendJogStop(uint8_t axis) {
  CommandMsg cmd{};
  cmd.cmd = CMD_JOG_STOP;
  cmd.axis = axis;
  uiSendCommand(cmd);
}

static void jogBtnEventCb(lv_event_t *e) {
  JogBtnCtx *ctx = (JogBtnCtx *)lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    sendJogStart(ctx->axis, ctx->dir);
    sLastJogSendMs[ctx->axis] = millis();
  } else if (code == LV_EVENT_PRESSING) {
    if (uiThrottle(sLastJogSendMs[ctx->axis], COMMAND_KEEPALIVE_INTERVAL_MS)) {
      sendJogStart(ctx->axis, ctx->dir);
    }
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    sendJogStop(ctx->axis);
  }
}

static void servoBtnEventCb(lv_event_t *e) {
  JogBtnCtx *ctx = (JogBtnCtx *)lv_event_get_user_data(e);
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  const StatusMsg &st = uiLink()->status();
  float current = st.axisPos[ctx->axis];
  CommandMsg cmd{};
  cmd.cmd = CMD_SERVO_SET;
  cmd.axis = ctx->axis;
  cmd.value = current + ctx->dir * SERVO_STEP_DEG;
  uiSendCommand(cmd);
}

static void speedBtnEventCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  sSpeedPct = (sSpeedPct == JOG_SPEED_SLOW_PCT) ? JOG_SPEED_FAST_PCT : JOG_SPEED_SLOW_PCT;
  lv_label_set_text_fmt(sSpeedBtnLabel, "Speed: %d%%", sSpeedPct);
}

static lv_obj_t *makeRow(lv_obj_t *parent, int y) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_set_size(row, SCREEN_WIDTH - 16, 24);
  lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, y);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  return row;
}

static lv_obj_t *makeSmallBtn(lv_obj_t *row, const char *text, lv_align_t align, int xOfs) {
  lv_obj_t *btn = lv_btn_create(row);
  lv_obj_set_size(btn, 34, 22);
  lv_obj_align(btn, align, xOfs, 0);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  return btn;
}

void uiJogBuild(lv_obj_t *parent) {
  sSpeedBtnLabel = nullptr;
  lv_obj_t *speedBtn = lv_btn_create(parent);
  lv_obj_set_size(speedBtn, 110, 22);
  lv_obj_align(speedBtn, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_add_event_cb(speedBtn, speedBtnEventCb, LV_EVENT_CLICKED, nullptr);
  sSpeedBtnLabel = lv_label_create(speedBtn);
  lv_label_set_text_fmt(sSpeedBtnLabel, "Speed: %d%%", sSpeedPct);
  lv_obj_center(sSpeedBtnLabel);

  int y = 26;
  for (uint8_t i = 0; i < AXIS_COUNT; i++) {
    lv_obj_t *row = makeRow(parent, y);

    lv_obj_t *nameLabel = lv_label_create(row);
    lv_label_set_text(nameLabel, kAxisNames[i]);
    lv_obj_align(nameLabel, LV_ALIGN_LEFT_MID, 2, 0);

    sValueLabel[i] = lv_label_create(row);
    lv_label_set_text(sValueLabel[i], "--");
    lv_obj_align(sValueLabel[i], LV_ALIGN_LEFT_MID, 60, 0);

    lv_obj_t *minusBtn = makeSmallBtn(row, "-", LV_ALIGN_RIGHT_MID, -40);
    lv_obj_t *plusBtn = makeSmallBtn(row, "+", LV_ALIGN_RIGHT_MID, 0);

    if (i < AXIS_STEPPER_COUNT) {
      sJogCtx[i * 2 + 0] = {i, -1};
      sJogCtx[i * 2 + 1] = {i, +1};
      lv_obj_add_event_cb(minusBtn, jogBtnEventCb, LV_EVENT_ALL, &sJogCtx[i * 2 + 0]);
      lv_obj_add_event_cb(plusBtn, jogBtnEventCb, LV_EVENT_ALL, &sJogCtx[i * 2 + 1]);
    } else {
      static JogBtnCtx servoCtx[4];
      uint8_t si = (i - AXIS_STEPPER_COUNT) * 2;
      servoCtx[si + 0] = {i, -1};
      servoCtx[si + 1] = {i, +1};
      lv_obj_add_event_cb(minusBtn, servoBtnEventCb, LV_EVENT_CLICKED, &servoCtx[si + 0]);
      lv_obj_add_event_cb(plusBtn, servoBtnEventCb, LV_EVENT_CLICKED, &servoCtx[si + 1]);
    }

    y += 26;
  }
}

void uiJogUpdate() {
  const StatusMsg &st = uiLink()->status();
  for (uint8_t i = 0; i < AXIS_COUNT; i++) {
    lv_label_set_text_fmt(sValueLabel[i], "%.1f", st.axisPos[i]);
  }
}
