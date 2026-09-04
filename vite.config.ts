import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
// base = "/Robot_Arm_V2/" vì đây là GitHub Pages project site
// (https://<user>.github.io/Robot_Arm_V2/), không phải user/org page.
export default defineConfig({
  plugins: [react()],
  base: '/Robot_Arm_V2/',
})
