# Robot Arm V2

Dự án cánh tay robot 6 bậc tự do (6-DOF) + 1 trục ray trượt, điều khiển bởi
3 vi xử lý ESP32 phối hợp (Wroom, C3 Super Mini, CYD). Chi tiết kiến trúc,
sơ đồ chân, giao thức và hướng dẫn nạp firmware xem tại
[`firmware/README.md`](firmware/README.md).

## Web — Bảng điều khiển (dashboard)

Thư mục gốc của repo là 1 ứng dụng web (React 19 + TypeScript + Vite +
Tailwind CSS v4) làm giao diện điều khiển robot từ trình duyệt.

- **Live demo (chỉ xem giao diện)**: https://kqviet1810.github.io/Robot_Arm_V2/
  (tự động build & deploy qua GitHub Actions mỗi khi có commit mới trên
  nhánh `main` — xem `.github/workflows/deploy-pages.yml`)
- **Điều khiển robot thật**: `http://192.168.4.1/` — join WiFi
  `RobotArmV2` (do ESP32 Wroom phát) rồi mở link này (ESP32 tự phục vụ
  trang, xem hướng dẫn upload ở [`firmware/README.md`](firmware/README.md#kết-nối-web-dashboard-websocket-thời-gian-thực)).
  **Không dùng link GitHub Pages để điều khiển thật** — khi nối WiFi của
  robot sẽ mất Internet nên không mở được trang đó, và trình duyệt cũng
  chặn kết nối `ws://` không mã hoá từ 1 trang `https://`.
- **Chạy local**:
  ```bash
  npm install
  npm run dev
  ```
- **Build production**: `npm run build` (kết quả ở `dist/` — copy vào
  `firmware/ESP32_Wroom_Main/data/` rồi upload LittleFS để cập nhật bản
  ESP32 tự phục vụ)
- **Kiểm tra code**: `npm run lint`

### Trạng thái: đã nối thật qua WebSocket

Web kết nối trực tiếp tới ESP32 Wroom qua WebSocket (`src/robot/espLink.ts`,
`ws://<ip>/ws`) — jog khớp, Home, Start/Stop, phát chu trình đều gửi lệnh
JSON thật và nhận lại vị trí/trạng thái sống từ robot (không còn chỉ log
cục bộ). Chi tiết giao thức, lý do dùng SoftAP thay vì nối qua GitHub Pages,
và giới hạn hiện tại (vd "GoHome: về vị trí trước đó" chưa nối logic thật)
— xem [`firmware/README.md`](firmware/README.md#kết-nối-web-dashboard-websocket-thời-gian-thực).

### Cấu trúc thư mục web

```
src/
  App.tsx, main.tsx        # điểm vào ứng dụng
  robot/
    main.tsx               # state tổng, ghép 3 tab, ket noi WebSocket that
    espLink.ts              # hook quan ly WebSocket toi ESP32 (ket noi/gui/nhan status)
    home.tsx                # tab Trang chủ
    control.tsx             # tab Điều khiển (jog khớp, servo, chu trình)
    setting.tsx              # tab Cài đặt (IP, GoHome)
    ui.tsx                    # component dùng chung (Card)
```
