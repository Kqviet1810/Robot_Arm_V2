#pragma once

// Include gộp cho thư viện dùng chung giữa 3 sketch:
//   ESP32_Wroom_Main, ESP32_C3_Sensor, ESP32_CYD_HMI
//
// Cài đặt: copy (hoặc symlink) thư mục firmware/libraries/RobotArmProtocol
// vào thư mục "libraries" trong Arduino sketchbook (Documents/Arduino/libraries),
// sau đó #include <RobotArmProtocol.h> trong mỗi sketch.

#include "RobotArmCommon.h"
#include "UartProtocol.h"
#include "EspNowProtocol.h"
