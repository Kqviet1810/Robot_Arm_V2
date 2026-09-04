# Robot Arm V2

Dự án cánh tay robot 6 bậc tự do (6-DOF) + 1 trục ray trượt, điều khiển bởi
3 vi xử lý ESP32 phối hợp (Wroom, C3 Super Mini, CYD). Chi tiết kiến trúc,
sơ đồ chân, giao thức và hướng dẫn nạp firmware xem tại
[`firmware/README.md`](firmware/README.md).

## Web — Bảng điều khiển (dashboard)

Thư mục gốc của repo là 1 ứng dụng web (React 19 + TypeScript + Vite +
Tailwind CSS v4) làm giao diện điều khiển robot từ trình duyệt.

- **Live demo**: https://kqviet1810.github.io/Robot_Arm_V2/
  (tự động build & deploy qua GitHub Actions mỗi khi có commit mới trên
  nhánh `main` — xem `.github/workflows/deploy-pages.yml`)
- **Chạy local**:
  ```bash
  npm install
  npm run dev
  ```
- **Build production**: `npm run build` (kết quả ở `dist/`)
- **Kiểm tra code**: `npm run lint`

### Trạng thái hiện tại: UI demo, CHƯA điều khiển robot thật

Toàn bộ nút bấm (jog khớp, Home, Start/Stop, lưu & phát chu trình...) đã
hoạt động đầy đủ về mặt giao diện và được ghi lại trong khung "Log điều
khiển", nhưng **chưa gửi bất kỳ lệnh nào qua mạng tới ESP32** — hàm
`writeEspPacket()` (`src/robot/main.tsx`) hiện chỉ log lại cục bộ. Để điều
khiển robot thật, cần thêm ở firmware (Wroom) một WiFi Access Point +
HTTP/WebSocket server nhận đúng định dạng lệnh JSON mà web đang gửi
(`{cmd, p, v, a, ...}`), việc này sẽ làm ở giai đoạn sau.

### Cấu trúc thư mục web

```
src/
  App.tsx, main.tsx        # điểm vào ứng dụng
  robot/
    main.tsx               # state tổng, ghép 3 tab, gửi/ghi log lệnh
    home.tsx                # tab Trang chủ
    control.tsx             # tab Điều khiển (jog khớp, servo, chu trình)
    setting.tsx              # tab Cài đặt (GoHome)
    ui.tsx                    # component dùng chung (Card)
```
