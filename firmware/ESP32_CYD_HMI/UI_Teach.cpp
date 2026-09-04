#include "UI_Teach.h"
#include "UI_Common.h"
#include "Config.h"
#include <RobotArmProtocol.h>
#include <string.h>

static lv_obj_t *sPointsLabel;
static lv_obj_t *sProgressLabel;
static lv_obj_t *sProgramDropdown;
static lv_obj_t *sLoopCheckbox;

static uint8_t selectedProgramIdx() {
  return sProgramDropdown ? (uint8_t)lv_dropdown_get_selected(sProgramDropdown) : 0;
}

static void sendSimple(uint8_t cmdType, uint8_t axis = AXIS_ALL) {
  CommandMsg cmd{};
  cmd.cmd = cmdType;
  cmd.axis = axis;
  uiSendCommand(cmd);
}

static void startTeachBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  sendSimple(CMD_TEACH_START);
}

static void stopTeachBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  sendSimple(CMD_TEACH_STOP);
}

static void savePointBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  sendSimple(CMD_TEACH_SAVE_POINT);
}

static void clearPointsBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  sendSimple(CMD_TEACH_CLEAR);
}

static void saveProgramBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  CommandMsg cmd{};
  cmd.cmd = CMD_TEACH_SAVE_PROGRAM;
  strncpy(cmd.programName, PROGRAM_SLOT_NAMES[selectedProgramIdx()], sizeof(cmd.programName) - 1);
  uiSendCommand(cmd);
}

static void playProgramBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  CommandMsg cmd{};
  cmd.cmd = CMD_PLAY_PROGRAM;
  strncpy(cmd.programName, PROGRAM_SLOT_NAMES[selectedProgramIdx()], sizeof(cmd.programName) - 1);
  uiSendCommand(cmd);
}

static void playPauseBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  sendSimple(CMD_PLAY_PAUSE);
}

static void stopPlayBtnCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  sendSimple(CMD_PLAY_STOP);
}

static void loopCheckboxCb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  sendSimple(CMD_PLAY_LOOP_TOGGLE);
}

static lv_obj_t *makeBtn(lv_obj_t *parent, const char *text, int w, int h, lv_align_t align, int x, int y,
                          lv_event_cb_t cb) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, w, h);
  lv_obj_align(btn, align, x, y);
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  return btn;
}

void uiTeachBuild(lv_obj_t *parent) {
  // Hàng 1: Start/Stop Teach + số điểm đã ghi
  makeBtn(parent, "Start Teach", 96, 26, LV_ALIGN_TOP_LEFT, 0, 0, startTeachBtnCb);
  makeBtn(parent, "Stop Teach", 96, 26, LV_ALIGN_TOP_LEFT, 100, 0, stopTeachBtnCb);
  sPointsLabel = lv_label_create(parent);
  lv_label_set_text(sPointsLabel, "Points: 0");
  lv_obj_align(sPointsLabel, LV_ALIGN_TOP_LEFT, 204, 6);

  // Hàng 2: Save Point + Clear
  makeBtn(parent, "Save Point", 96, 26, LV_ALIGN_TOP_LEFT, 0, 30, savePointBtnCb);
  makeBtn(parent, "Clear", 96, 26, LV_ALIGN_TOP_LEFT, 100, 30, clearPointsBtnCb);

  // Hàng 3: chọn chương trình + Save
  sProgramDropdown = lv_dropdown_create(parent);
  lv_obj_set_size(sProgramDropdown, 100, 26);
  lv_obj_align(sProgramDropdown, LV_ALIGN_TOP_LEFT, 0, 60);
  {
    String opts;
    for (uint8_t i = 0; i < PROGRAM_SLOT_COUNT; i++) {
      opts += PROGRAM_SLOT_NAMES[i];
      if (i + 1 < PROGRAM_SLOT_COUNT) opts += "\n";
    }
    lv_dropdown_set_options(sProgramDropdown, opts.c_str());
  }
  makeBtn(parent, "Save Prog", 96, 26, LV_ALIGN_TOP_LEFT, 108, 60, saveProgramBtnCb);
  makeBtn(parent, "Load & Play", 96, 26, LV_ALIGN_TOP_LEFT, 208, 60, playProgramBtnCb);

  // Hàng 4: Play/Pause, Stop, Loop, tiến trình
  makeBtn(parent, "Play/Pause", 96, 26, LV_ALIGN_TOP_LEFT, 0, 90, playPauseBtnCb);
  makeBtn(parent, "Stop", 96, 26, LV_ALIGN_TOP_LEFT, 100, 90, stopPlayBtnCb);

  sLoopCheckbox = lv_checkbox_create(parent);
  lv_checkbox_set_text(sLoopCheckbox, "Loop");
  lv_obj_align(sLoopCheckbox, LV_ALIGN_TOP_LEFT, 204, 96);
  lv_obj_add_event_cb(sLoopCheckbox, loopCheckboxCb, LV_EVENT_VALUE_CHANGED, nullptr);

  sProgressLabel = lv_label_create(parent);
  lv_label_set_text(sProgressLabel, "Playback: -/-");
  lv_obj_align(sProgressLabel, LV_ALIGN_TOP_LEFT, 0, 122);
}

void uiTeachUpdate() {
  const StatusMsg &st = uiLink()->status();
  lv_label_set_text_fmt(sPointsLabel, "Points: %d", st.waypointCount);
  lv_label_set_text_fmt(sProgressLabel, "Playback: %d/%d%s", st.playIndex, st.playTotal,
                         st.state == STATE_TEACH_PLAY ? "" : " (stopped)");
}
