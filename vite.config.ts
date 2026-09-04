import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
// base tương đối ('./') để MỘT bản build chạy đúng ở cả 2 nơi:
//   - GitHub Pages: https://<user>.github.io/Robot_Arm_V2/
//   - ESP32 Wroom tự phục vụ (LittleFS): http://192.168.4.1/
// (không có SPA router trong app này nên đường dẫn tương đối luôn đúng
// miễn URL có dấu "/" cuối, đúng cho cả 2 trường hợp trên).
export default defineConfig({
  plugins: [react()],
  base: './',
})
