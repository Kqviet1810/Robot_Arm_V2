#include "UI_Home.h"
#include "UI_Common.h"
#include <RobotArmProtocol.h>

static lv_obj_t *sPosLabel;

static void homeAllBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  CommandMsg cmd{};
  cmd.cmd = CMD_HOME_ALL;
  cmd.axis = AXIS_ALL;
  uiSendCommand(cmd);
}

static void clearErrorBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  CommandMsg cmd{};
  cmd.cmd = CMD_CLEAR_ERROR;
  uiSendCommand(cmd);
}

void uiHomeBuild(lv_obj_t *parent) {
  lv_obj_t *homeBtn = lv_btn_create(parent);
  lv_obj_set_size(homeBtn, 140, 44);
  lv_obj_align(homeBtn, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_add_event_cb(homeBtn, homeAllBtnCb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *homeLabel = lv_label_create(homeBtn);
  lv_label_set_text(homeLabel, "HOME ALL");
  lv_obj_center(homeLabel);

  lv_obj_t *clearBtn = lv_btn_create(parent);
  lv_obj_set_size(clearBtn, 140, 44);
  lv_obj_align(clearBtn, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(clearBtn, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_add_event_cb(clearBtn, clearErrorBtnCb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *clearLabel = lv_label_create(clearBtn);
  lv_label_set_text(clearLabel, "CLEAR ERROR");
  lv_obj_center(clearLabel);

  sPosLabel = lv_label_create(parent);
  lv_obj_align(sPosLabel, LV_ALIGN_TOP_LEFT, 0, 54);
  lv_label_set_text(sPosLabel, "X:--  Y:--  Z:--\nA:--  B:--  C:--");
}

void uiHomeUpdate() {
  const StatusMsg &st = uiLink()->status();
  char buf[96];
  snprintf(buf, sizeof(buf), "X:%.1f  Y:%.1f  Z:%.1f\nA:%.1f  B:%.1f  C:%.1f",
           st.axisPos[AXIS_X], st.axisPos[AXIS_Y], st.axisPos[AXIS_Z],
           st.axisPos[AXIS_A], st.axisPos[AXIS_B], st.axisPos[AXIS_C]);
  lv_label_set_text(sPosLabel, buf);
}
