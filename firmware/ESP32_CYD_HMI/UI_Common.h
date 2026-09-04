#pragma once
#include <lvgl.h>
#include <RobotArmProtocol.h>
#include "LinkWroom.h"

// Hạ tầng dùng chung cho giao diện: thanh trạng thái trên cùng, nút E-STOP
// cố định, thanh điều hướng dưới cùng, và vùng nội dung được thay đổi theo
// từng trang (Home/Jog/Teach/Diagnostics).

enum UiPage { PAGE_HOME = 0, PAGE_JOG = 1, PAGE_TEACH = 2, PAGE_DIAG = 3 };

void uiInit(LinkWroom *link);
void uiShowPage(UiPage page);
void uiTick();  // gọi định kỳ trong loop() để làm mới dữ liệu hiển thị

// Dùng chung bởi các trang: gửi lệnh tới Wroom với seq tự tăng.
void uiSendCommand(CommandMsg cmd);
LinkWroom *uiLink();
lv_obj_t *uiContent();  // container nội dung của trang hiện tại, các trang build UI vào đây

// Giới hạn tần suất gửi lệnh lặp lại (vd giữ nút jog) theo khoảng thời gian tối thiểu.
bool uiThrottle(uint32_t &lastMs, uint32_t intervalMs);
