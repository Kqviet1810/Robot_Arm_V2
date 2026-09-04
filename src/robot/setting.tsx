import type { Dispatch, SetStateAction } from "react";
import { Card } from "./ui";

export type GoHomeMode = "esp_default" | "previous";

type SettingTabProps = {
  ip: string;
  port: string;
  goHomeMode: GoHomeMode;
  setGoHomeMode: (mode: GoHomeMode) => void;
  goHomeSpeed: number;
  setGoHomeSpeed: Dispatch<SetStateAction<number>>;
  goHomeSmooth: boolean;
  setGoHomeSmooth: Dispatch<SetStateAction<boolean>>;
};

export default function SettingTab({
  ip,
  port,
  goHomeMode,
  setGoHomeMode,
  goHomeSpeed,
  setGoHomeSpeed,
  goHomeSmooth,
  setGoHomeSmooth,
}: SettingTabProps) {
  return (
    <div className="grid min-h-0 gap-4 overflow-hidden xl:grid-cols-[minmax(0,1fr)_380px]">
      <Card title="Cai dat chung">
        <div className="grid gap-3 md:grid-cols-2">
          <div className="rounded-xl bg-white/5 p-3 text-sm text-white/75">IP mac dinh: {ip}</div>
          <div className="rounded-xl bg-white/5 p-3 text-sm text-white/75">Port mac dinh: {port}</div>
        </div>
      </Card>

      <Card title="Thiet lap GoHome">
        <div className="space-y-3">
          <div>
            <div className="mb-2 text-sm text-white/70">Kieu ve Home</div>
            <div className="space-y-2">
              <label className="flex items-center gap-2 text-sm text-white/80">
                <input
                  type="radio"
                  name="gohome_mode"
                  checked={goHomeMode === "esp_default"}
                  onChange={() => setGoHomeMode("esp_default")}
                />
                Dung goc mac dinh do ESP32 quan ly
              </label>
              <label className="flex items-center gap-2 text-sm text-white/80">
                <input
                  type="radio"
                  name="gohome_mode"
                  checked={goHomeMode === "previous"}
                  onChange={() => setGoHomeMode("previous")}
                />
                Ve vi tri truoc do
              </label>
            </div>
          </div>

          <div className="rounded-xl bg-white/4 p-3">
            <div className="mb-1 text-sm text-white/70">Toc do GoHome</div>
            <input
              type="range"
              min={5}
              max={120}
              value={goHomeSpeed}
              onChange={(event) => setGoHomeSpeed(Number(event.target.value))}
              className="w-full accent-emerald-500"
            />
            <div className="mt-1 text-sm text-white/85">{goHomeSpeed} deg/s</div>
          </div>

          <label className="flex items-center gap-2 text-sm text-white/80">
            <input
              type="checkbox"
              checked={goHomeSmooth}
              onChange={() => setGoHomeSmooth((prev) => !prev)}
            />
            Di chuyen muot khi GoHome
          </label>
        </div>
      </Card>
    </div>
  );
}
