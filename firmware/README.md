# Robot Arm V2 — Firmware 3 vi xử lý

Cánh tay robot 6 bậc (4 trục stepper X/Y/Z/A qua TMC2209 + 2 servo SG90 B/C,
trong đó A là trục ray trượt dùng đai GT2), điều khiển bởi 3 board ESP32:

| Board | Vai trò | Giao tiếp |
|---|---|---|
| ESP32 Wroom (Main) | Điều khiển 4 stepper (TMC2209/UART) + 2 servo SG90 | UART ↔ C3, ESP-NOW ↔ CYD |
| ESP32 C3 Super Mini | Đọc 4 cảm biến AS5600 qua TCA9548A | UART ↔ Wroom, ESP-NOW → CYD |
| ESP32 CYD (ESP32-2432S028, 2.8" ILI9341 + XPT2046) | Màn hình điều khiển (HMI) | ESP-NOW ↔ Wroom/C3 |

Phạm vi giai đoạn hiện tại: dựng khung giao tiếp giữa 3 board, điều khiển
từng trục cơ bản (jog + home bằng StallGuard, không tính toán động học
nghịch), và tính năng **cầm tay chỉ việc** (hand-guide teach & playback).

## Cài đặt thư viện dùng chung

`libraries/RobotArmProtocol` định nghĩa struct/frame dùng chung giữa cả 3
sketch (UART frame Wroom↔C3, gói tin ESP-NOW). Copy thư mục này vào
`Documents/Arduino/libraries/` (Windows/macOS) hoặc `~/Arduino/libraries/`
(Linux) trước khi biên dịch bất kỳ sketch nào, rồi `#include <RobotArmProtocol.h>`.

## Thư viện Arduino cần cài (Library Manager / Boards Manager)

- ESP32 board package (Espressif) — cho cả 3 board (Wroom, C3 Super Mini, CYD đều dùng chip ESP32/ESP32-C3).
- **TMCStepper** (teemuatlut) — cấu hình UART TMC2209.
- **FastAccelStepper** — sinh xung STEP/DIR hiệu năng cao, nhiều trục đồng thời.
- **ESP32Servo** — điều khiển servo SG90.
- **TFT_eSPI** — driver màn hình ILI9341 (CYD). Cấu hình chân đã được nạp sẵn
  trong sketch qua `ESP32_CYD_HMI/TFT_eSPI_Setup.h` (dùng `USER_SETUP_LOADED`),
  không cần sửa `User_Setup.h` trong thư mục thư viện.
- **lvgl** (v8.x) — giao diện HMI. **Bắt buộc** có file `lv_conf.h`: sau khi
  cài thư viện lvgl, copy `lv_conf_template.h` (trong thư mục thư viện lvgl)
  ra `Documents/Arduino/libraries/lv_conf.h` (ngang hàng thư mục `lvgl/`),
  đổi dòng `#if 0` đầu file thành `#if 1`, rồi đặt `#define LV_COLOR_DEPTH 16`
  (khớp với `TFT_eSPI`/ILI9341). Có thể bật thêm `LV_USE_DROPDOWN`,
  `LV_USE_CHECKBOX` nếu bản mặc định của bạn đang tắt (mặc định template đã bật sẵn).
- **XPT2046_Touchscreen** (Paul Stoffregen) — cảm ứng điện trở trên CYD, dùng
  chung bus SPI mặc định với TFT (MISO12/MOSI13/SCLK14 cũng là chân VSPI mặc
  định của ESP32 nên không cần cấu hình SPI riêng).

> Sau khi nạp lần đầu, mở Serial Monitor và chạm 4 góc màn hình để xem giá
> trị thô từ `ts.getPoint()` (có thể tạm thêm `Serial.println` trong
> `touchpadRead()`), rồi cập nhật `TOUCH_RAW_X_MIN/MAX`, `TOUCH_RAW_Y_MIN/MAX`
> trong `ESP32_CYD_HMI/Config.h` cho khớp — cảm ứng điện trở lệch khá nhiều
> giữa các lô board.

## Sơ đồ chân

### ESP32 Wroom (Main)

| Chức năng | GPIO | Ghi chú |
|---|---|---|
| Motor X — STEP / DIR | 14 / 27 | |
| Motor Y — STEP / DIR | 26 / 25 | |
| Motor Z — STEP / DIR | 18 / 4 | Z = xoay toa chính |
| Motor A — STEP / DIR | 21 / 19 | A = trục ray (đai GT2-20T) |
| DIAG X / Y / Z / A (StallGuard) | 35 / 34 / 36 / 39 | Input-only, không dùng làm limit switch cơ khí |
| UART1 → bus TMC2209 (multi-drop) | TX 23 / RX 22 | 115200, địa chỉ MS1/MS2: 0=X,1=Y,2=Z,3=A |
| UART2 → ESP32 C3 | TX 17 / RX 16 | 115200 |
| Servo B (xoay kẹp) | 32 | SG90, 0-180° |
| Servo C (kẹp) | 33 | SG90, 0-180° |

> **Đã sửa 2 xung đột so với bản pin gốc**: (1) GPIO4 trước đây vừa là
> Direction Pin của Motor Z vừa là RXD UART tới ESP32 C3 — nay UART tới C3
> chuyển sang dùng UART2 sẵn có (TX17/RX16). (2) GPIO3 (TXD tới C3 theo bản
> cũ) trùng UART0 dùng cho nạp code/Serial Monitor qua USB — không còn dùng.
> (3) 4 chân "Limit Pin" (35/34/36/39) được giữ nguyên vị trí vật lý nhưng
> đổi công năng thành DIAG (StallGuard) thay vì công tắc hành trình cơ khí,
> theo đúng cách homing sensorless bằng StallGuard (SGTHRS X/Y/Z=30, A=40).

### ESP32 C3 Super Mini

| Chức năng | GPIO (đề xuất) |
|---|---|
| I2C SDA / SCL → TCA9548A (địa chỉ 0x70) | 8 / 9 |
| UART → Wroom: TX / RX | 6 / 7 |

TCA9548A channel 0→X, 1→Y, 2→Z, 3→A; mỗi channel gắn 1 AS5600 (địa chỉ I2C
cố định 0x36). Các chân trên là mặc định phần mềm (`Config.h`), có thể đổi
tùy board thực tế — tránh dùng GPIO9 nếu board strap boot khác biệt.

### ESP32 CYD (ESP32-2432S028)

Dùng pinout chuẩn cộng đồng cho board này (không tự suy ra để tránh sai
lệch giữa các lô sản xuất):

| Chức năng | GPIO |
|---|---|
| TFT MOSI / MISO / SCLK | 13 / 12 / 14 |
| TFT CS / DC / RST | 15 / 2 / -1 (nối EN) |
| TFT Backlight | 21 |
| Touch (XPT2046) CS / IRQ | 33 / 36 |

## Cấu trúc thư mục

```
firmware/
  libraries/RobotArmProtocol/   # thư viện giao thức dùng chung (xem trên)
  ESP32_Wroom_Main/             # sketch board điều khiển chính
  ESP32_C3_Sensor/               # sketch board đọc cảm biến AS5600
  ESP32_CYD_HMI/                  # sketch màn hình điều khiển
```

Mỗi sketch có `Config.h` riêng chứa pin map, tỉ số truyền, hằng số điều
chỉnh — sửa ở đó khi cần tinh chỉnh phần cứng thực tế, không cần sửa logic.

## Giao thức

- **UART Wroom ↔ C3**: frame nhị phân `AA 55 LEN TYPE PAYLOAD CRC8`
  (xem `UartProtocol.h`). C3 gửi `AngleReport` (góc AS5600 4 trục) định kỳ.
- **ESP-NOW** (broadcast, không cần pair MAC — xem `EspNowProtocol.h`):
  - C3 → CYD: `TelemetryMsg` (góc thô AS5600, ~10Hz, giám sát).
  - Wroom → CYD: `StatusMsg` (trạng thái hệ thống, vị trí lệnh, lỗi/stall, SG_RESULT).
  - CYD → Wroom: `CommandMsg` (jog, home, teach/playback, E-Stop).

## Trạng thái triển khai

- [x] Khung thư mục + thư viện giao thức dùng chung
- [x] ESP32_C3_Sensor: đọc AS5600/TCA9548A, UART, ESP-NOW telemetry
- [x] ESP32_Wroom_Main: TMC2209/StallGuard homing, servo, teach & playback, ESP-NOW/UART
- [x] ESP32_CYD_HMI: giao diện LVGL (Home/Jog/Teach/Diagnostics), ESP-NOW
- [ ] Biên dịch thử bằng arduino-cli: môi trường phát triển này bị chặn tải
      gói `esp32:esp32` (mạng ra ngoài giới hạn theo whitelist), nên KHÔNG
      build-check được tự động. Code đã được rà soát thủ công kỹ theo API
      của từng thư viện (TMCStepper, FastAccelStepper, ESP32Servo, lvgl v8,
      TFT_eSPI, esp_now) nhưng vẫn cần mở bằng Arduino IDE (đã cài đủ thư
      viện ở trên) và bấm Verify cho cả 3 sketch trước khi nạp vào board.
- [ ] Kiểm thử trên phần cứng thật (chưa thể test trong môi trường này — cần nạp và chạy thử trên robot thật)

## Vấn đề đã biết / cần lưu ý khi test thực tế

- Dòng điện motor (`TMC_RMS_CURRENT_MA`), điện trở sense (`TMC_R_SENSE`),
  ngưỡng tốc độ StallGuard (`TCOOLTHRS_VALUE`) và soft-limit hành trình từng
  trục (`AXIS_SOFT_MIN/MAX`) trong `ESP32_Wroom_Main/Config.h` đều là giá trị
  khởi điểm, CẦN chỉnh lại theo động cơ/cơ khí thật (StallGuard rất nhạy với
  tốc độ/dòng điện, nếu home không kích hoạt được hoặc kích hoạt sai lúc chạy
  bình thường thì tăng/giảm SGTHRS hoặc TCOOLTHRS).
- Homing hiện chạy tuần tự và BLOCKING từng trục (Home All = home lần lượt
  X→Y→Z→A), không chạy song song — đơn giản và an toàn hơn cho giai đoạn này
  nhưng sẽ chậm hơn nếu sau này cần tối ưu thời gian home.
- lv_conf.h phải tồn tại và đặt đúng `LV_COLOR_DEPTH 16` (xem mục thư viện ở trên), nếu không sketch CYD sẽ không biên dịch được.
- Hiệu chỉnh vùng chạm XPT2046 (`TOUCH_RAW_X/Y_MIN/MAX` trong `ESP32_CYD_HMI/Config.h`) theo board thật.
