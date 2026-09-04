#include "UI_Diagnostics.h"
#include "UI_Common.h"
#include "Config.h"
#include <RobotArmProtocol.h>

static lv_obj_t *sSgLabel;
static lv_obj_t *sAngleLabel;
static lv_obj_t *sFaultLabel;
static lv_obj_t *sLinkLabel;

void uiDiagBuild(lv_obj_t *parent) {
  lv_obj_t *title1 = lv_label_create(parent);
  lv_label_set_text(title1, "StallGuard SG_RESULT (X Y Z A):");
  lv_obj_align(title1, LV_ALIGN_TOP_LEFT, 0, 0);
  sSgLabel = lv_label_create(parent);
  lv_obj_align(sSgLabel, LV_ALIGN_TOP_LEFT, 0, 16);

  lv_obj_t *title2 = lv_label_create(parent);
  lv_label_set_text(title2, "AS5600 raw deg / ok (X Y Z A):");
  lv_obj_align(title2, LV_ALIGN_TOP_LEFT, 0, 40);
  sAngleLabel = lv_label_create(parent);
  lv_obj_align(sAngleLabel, LV_ALIGN_TOP_LEFT, 0, 56);

  lv_obj_t *title3 = lv_label_create(parent);
  lv_label_set_text(title3, "Fault mask (bit X..A):");
  lv_obj_align(title3, LV_ALIGN_TOP_LEFT, 0, 84);
  sFaultLabel = lv_label_create(parent);
  lv_obj_align(sFaultLabel, LV_ALIGN_TOP_LEFT, 0, 100);

  sLinkLabel = lv_label_create(parent);
  lv_obj_align(sLinkLabel, LV_ALIGN_TOP_LEFT, 0, 128);
}

void uiDiagUpdate() {
  LinkWroom *link = uiLink();
  const StatusMsg &st = link->status();
  const TelemetryMsg &tm = link->telemetry();

  char buf[80];
  snprintf(buf, sizeof(buf), "%d  %d  %d  %d", st.sgResult[0], st.sgResult[1], st.sgResult[2], st.sgResult[3]);
  lv_label_set_text(sSgLabel, buf);

  snprintf(buf, sizeof(buf), "%.0f/%d  %.0f/%d  %.0f/%d  %.0f/%d", tm.angleDeg[0],
           (tm.sensorOk >> 0) & 1, tm.angleDeg[1], (tm.sensorOk >> 1) & 1, tm.angleDeg[2],
           (tm.sensorOk >> 2) & 1, tm.angleDeg[3], (tm.sensorOk >> 3) & 1);
  lv_label_set_text(sAngleLabel, buf);

  snprintf(buf, sizeof(buf), "0x%02X", st.axisFault);
  lv_label_set_text(sFaultLabel, buf);

  snprintf(buf, sizeof(buf), "Link Wroom: %s | Link C3: %s", link->wroomLinkFresh() ? "song" : "mat",
           link->c3LinkFresh() ? "song" : "mat");
  lv_label_set_text(sLinkLabel, buf);
}
