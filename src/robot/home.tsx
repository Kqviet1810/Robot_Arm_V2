import { Activity, Wifi, WifiOff, Zap } from "lucide-react";
import type { ComponentType } from "react";
import { Card } from "./ui";

type Tone = "default" | "good" | "warn";

type HomeTabProps = {
  connected: boolean;
  jointSpeed: number;
  poseSummary: string;
};

function TinyStat({
  icon: Icon,
  label,
  value,
  tone = "default",
}: {
  icon: ComponentType<{ className?: string }>;
  label: string;
  value: string;
  tone?: Tone;
}) {
  const toneClass =
    tone === "good"
      ? "border-emerald-400/20 bg-emerald-500/10 text-emerald-300"
      : tone === "warn"
      ? "border-red-400/20 bg-red-500/10 text-red-300"
      : "border-white/10 bg-white/5 text-white/80";

  return (
    <div className={`rounded-xl border px-3 py-2 ${toneClass}`}>
      <div className="flex items-center gap-1.5 text-[11px]">
        <Icon className="h-3.5 w-3.5" />
        <span>{label}</span>
      </div>
      <div className="mt-1 break-all text-sm font-semibold">{value}</div>
    </div>
  );
}

export default function HomeTab({ connected, jointSpeed, poseSummary }: HomeTabProps) {
  return (
    <div className="min-h-0 overflow-hidden">
      <Card title="Trang chu">
        <div className="grid gap-3 md:grid-cols-3">
          <TinyStat
            icon={connected ? Wifi : WifiOff}
            label="Ket noi"
            value={connected ? "Online" : "Offline"}
            tone={connected ? "good" : "warn"}
          />
          <TinyStat icon={Zap} label="Toc do hien tai" value={`${jointSpeed} deg/s`} />
          <TinyStat icon={Activity} label="Pose hien tai" value={poseSummary} />
        </div>
      </Card>
    </div>
  );
}
