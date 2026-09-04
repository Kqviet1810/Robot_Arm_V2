#include "UI_Common.h"
#include "Config.h"
#include "UI_Home.h"
#include "UI_Jog.h"
#include "UI_Teach.h"
#include "UI_Diagnostics.h"

static LinkWroom *sLink = nullptr;
static uint32_t sSeq = 0;
static UiPage sCurrentPage = PAGE_HOME;

static lv_obj_t *sTopBar;
static lv_obj_t *sLinkLabel;
static lv_obj_t *sModeLabel;
static lv_obj_t *sContent;
static lv_obj_t *sNavBar;
static lv_obj_t *sNavBtn[4];
static lv_obj_t *sNavLabel[4];

static const char *kStateNames[] = {"IDLE", "HOMING", "JOG", "TEACH", "PLAY", "ERROR"};
static const char *kPageNames[] = {"Home", "Jog", "Teach", "Diag"};

bool uiThrottle(uint32_t &lastMs, uint32_t intervalMs) {
  uint32_t now = millis();
  if (now - lastMs >= intervalMs) {
    lastMs = now;
    return true;
  }
  return false;
}

void uiSendCommand(CommandMsg cmd) {
  cmd.deviceId = DEV_CYD;
  cmd.msgType = ESPNOW_MSG_COMMAND;
  cmd.seq = sSeq++;
  if (sLink) sLink->sendCommand(cmd);
}

LinkWroom *uiLink() { return sLink; }
lv_obj_t *uiContent() { return sContent; }

static void estopBtnEventCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  CommandMsg cmd{};
  cmd.cmd = CMD_ESTOP;
  cmd.axis = AXIS_ALL;
  uiSendCommand(cmd);
}

static void navBtnEventCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiPage page = (UiPage)(intptr_t)lv_event_get_user_data(e);
  uiShowPage(page);
}

static void buildTopBar(lv_obj_t *root) {
  sTopBar = lv_obj_create(root);
  lv_obj_set_size(sTopBar, SCREEN_WIDTH - 40, 22);
  lv_obj_align(sTopBar, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_pad_all(sTopBar, 2, 0);
  lv_obj_clear_flag(sTopBar, LV_OBJ_FLAG_SCROLLABLE);

  sLinkLabel = lv_label_create(sTopBar);
  lv_label_set_text(sLinkLabel, "Wroom:-- C3:--");
  lv_obj_align(sLinkLabel, LV_ALIGN_LEFT_MID, 0, 0);

  sModeLabel = lv_label_create(sTopBar);
  lv_label_set_text(sModeLabel, "IDLE");
  lv_obj_align(sModeLabel, LV_ALIGN_RIGHT_MID, 0, 0);

  lv_obj_t *estopBtn = lv_btn_create(root);
  lv_obj_set_size(estopBtn, 40, 22);
  lv_obj_align(estopBtn, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(estopBtn, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_add_event_cb(estopBtn, estopBtnEventCb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *estopLabel = lv_label_create(estopBtn);
  lv_label_set_text(estopLabel, "STOP");
  lv_obj_center(estopLabel);
}

static void buildNavBar(lv_obj_t *root) {
  sNavBar = lv_obj_create(root);
  lv_obj_set_size(sNavBar, SCREEN_WIDTH, 36);
  lv_obj_align(sNavBar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_pad_all(sNavBar, 2, 0);
  lv_obj_set_flex_flow(sNavBar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(sNavBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(sNavBar, LV_OBJ_FLAG_SCROLLABLE);

  for (int i = 0; i < 4; i++) {
    sNavBtn[i] = lv_btn_create(sNavBar);
    lv_obj_set_size(sNavBtn[i], 74, 30);
    lv_obj_add_event_cb(sNavBtn[i], navBtnEventCb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    sNavLabel[i] = lv_label_create(sNavBtn[i]);
    lv_label_set_text(sNavLabel[i], kPageNames[i]);
    lv_obj_center(sNavLabel[i]);
  }
}

void uiInit(LinkWroom *link) {
  sLink = link;

  lv_obj_t *root = lv_scr_act();
  buildTopBar(root);
  buildNavBar(root);

  sContent = lv_obj_create(root);
  lv_obj_set_size(sContent, SCREEN_WIDTH, SCREEN_HEIGHT - 22 - 36);
  lv_obj_align(sContent, LV_ALIGN_TOP_MID, 0, 22);
  lv_obj_set_style_pad_all(sContent, 4, 0);

  uiShowPage(PAGE_HOME);
}

void uiShowPage(UiPage page) {
  sCurrentPage = page;
  lv_obj_clean(sContent);
  switch (page) {
    case PAGE_HOME: uiHomeBuild(sContent); break;
    case PAGE_JOG: uiJogBuild(sContent); break;
    case PAGE_TEACH: uiTeachBuild(sContent); break;
    case PAGE_DIAG: uiDiagBuild(sContent); break;
  }
  for (int i = 0; i < 4; i++) {
    if (i == (int)page) lv_obj_set_style_bg_color(sNavBtn[i], lv_palette_main(LV_PALETTE_BLUE), 0);
    else lv_obj_set_style_bg_color(sNavBtn[i], lv_palette_main(LV_PALETTE_GREY), 0);
  }
}

void uiTick() {
  if (!sLink) return;
  const StatusMsg &st = sLink->status();

  char buf[40];
  snprintf(buf, sizeof(buf), "Wroom:%s C3:%s", sLink->wroomLinkFresh() ? "OK" : "--",
           sLink->c3LinkFresh() ? "OK" : "--");
  lv_label_set_text(sLinkLabel, buf);

  uint8_t stateIdx = st.state <= STATE_ERROR ? st.state : 0;
  lv_label_set_text(sModeLabel, kStateNames[stateIdx]);
  lv_obj_set_style_text_color(sModeLabel,
      st.state == STATE_ERROR ? lv_palette_main(LV_PALETTE_RED) : lv_color_black(), 0);

  switch (sCurrentPage) {
    case PAGE_HOME: uiHomeUpdate(); break;
    case PAGE_JOG: uiJogUpdate(); break;
    case PAGE_TEACH: uiTeachUpdate(); break;
    case PAGE_DIAG: uiDiagUpdate(); break;
  }
}
