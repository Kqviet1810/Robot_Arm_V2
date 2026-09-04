import {
  ChevronLeft,
  ChevronRight,
  CircleDot,
  Eye,
  Grip,
  Home,
  Play,
  RotateCw,
  Save,
  Settings2,
  Square,
  Trash2,
  Wifi,
  WifiOff,
  Zap,
} from "lucide-react";
import type { ComponentType } from "react";
import { Card } from "./ui";

export type Mode = "live" | "batch";

export type SavedCycle = {
  id: number;
  name: string;
  points: number[][];
  speed: number;
  createdAt: string;
};

export type PayloadLog = {
  time: string;
  payload: string;
};

type Tone = "default" | "good" | "warn";

type ControlTabProps = {
  actionLocked: boolean;
  controlsLocked: boolean;
  connected: boolean;
  joints: number[];
  controlMode: Mode;
  jointSpeed: number;
  setJointSpeed: (value: number) => void;
  jointAccel: number;
  setJointAccel: (value: number) => void;
  jogStep: number;
  setJogStep: (value: number) => void;
  cycleName: string;
  setCycleName: (value: string) => void;
  saveError: string;
  saveSuccess: string;
  recordingActive: boolean;
  draftPoints: number[][];
  cycleViewId: number | null;
  cycles: SavedCycle[];
  queueIds: number[];
  queueSelectMode: boolean;
  queuedCycles: SavedCycle[];
  focusDraftMode: boolean;
  poseSummary: string;
  lastCommandLabel: string;
  logs: PayloadLog[];
  onNudgeJoint: (index: number, delta: number) => void;
  onJointChange: (index: number, value: number) => void;
  onHome: () => void;
  onSendPose: () => void;
  onStart: () => void;
  onStop: () => void;
  onResetDraft: () => void;
  onStartRecording: () => void;
  onSaveCurrentPoint: () => void;
  onStopAndSaveCycle: () => void;
  onRunCycle: (cycle: SavedCycle) => void;
  onPreviewCycle: (id: number) => void;
  onDeleteCycle: (id: number) => void;
  onToggleQueue: (id: number) => void;
  onRunQueue: (items: SavedCycle[]) => void;
  onToggleQueueMode: () => void;
  onClearSaveNotice: () => void;
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

function AxisCard({
  title,
  value,
  step,
  onNudge,
  onChange,
  disabled = false,
}: {
  title: string;
  value: number;
  step: number;
  onNudge: (delta: number) => void;
  onChange: (value: number) => void;
  disabled?: boolean;
}) {
  return (
    <div className={`rounded-2xl bg-white/5 px-3 py-3 ${disabled ? "opacity-45" : ""}`}>
      <div className="mb-2 flex items-center justify-between">
        <div className="text-sm font-medium text-white">{title}</div>
        <div className="font-mono text-sm text-white/85">{Math.round(value)} deg</div>
      </div>

      <div className="flex items-center gap-2">
        <button
          disabled={disabled}
          onClick={() => onNudge(-step)}
          className="rounded-lg bg-white/8 px-2 py-1 text-white/80 hover:bg-white/12 disabled:cursor-not-allowed disabled:opacity-40"
        >
          <ChevronLeft className="h-4 w-4" />
        </button>
        <input
          type="range"
          min={-180}
          max={180}
          step={1}
          value={value}
          disabled={disabled}
          onChange={(event) => onChange(Number(event.target.value))}
          className="flex-1 accent-emerald-500 disabled:opacity-40"
        />
        <button
          disabled={disabled}
          onClick={() => onNudge(step)}
          className="rounded-lg bg-white/8 px-2 py-1 text-white/80 hover:bg-white/12 disabled:cursor-not-allowed disabled:opacity-40"
        >
          <ChevronRight className="h-4 w-4" />
        </button>
      </div>
    </div>
  );
}

function ServoCard({
  title,
  hint,
  icon: Icon,
  value,
  onChange,
  presets,
  disabled = false,
}: {
  title: string;
  hint: string;
  icon: ComponentType<{ className?: string }>;
  value: number;
  onChange: (value: number) => void;
  presets: number[];
  disabled?: boolean;
}) {
  return (
    <div className={`rounded-2xl bg-white/5 p-3 ${disabled ? "opacity-45" : ""}`}>
      <div className="mb-2 flex items-center justify-between">
        <div className="flex items-center gap-2 text-sm font-medium text-white">
          <Icon className="h-4 w-4 text-emerald-300" />
          {title}
        </div>
        <div className="font-mono text-sm text-white/85">{Math.round(value)} deg</div>
      </div>

      <div className="mb-2 text-[11px] text-white/45">{hint}</div>

      <div className="mb-3 flex items-center gap-2">
        <button
          disabled={disabled}
          onClick={() => onChange(Math.max(0, value - 5))}
          className="rounded-lg bg-white/8 px-2 py-1 text-xs text-white/80 hover:bg-white/12 disabled:cursor-not-allowed disabled:opacity-40"
        >
          -5
        </button>
        <input
          type="range"
          min={0}
          max={180}
          step={1}
          value={value}
          disabled={disabled}
          onChange={(event) => onChange(Number(event.target.value))}
          className="flex-1 accent-emerald-500 disabled:opacity-40"
        />
        <button
          disabled={disabled}
          onClick={() => onChange(Math.min(180, value + 5))}
          className="rounded-lg bg-white/8 px-2 py-1 text-xs text-white/80 hover:bg-white/12 disabled:cursor-not-allowed disabled:opacity-40"
        >
          +5
        </button>
      </div>

      <div className="grid grid-cols-4 gap-2">
        {presets.map((preset) => (
          <button
            key={preset}
            disabled={disabled}
            onClick={() => onChange(preset)}
            className={`rounded-lg px-2 py-1.5 text-xs transition disabled:cursor-not-allowed disabled:opacity-40 ${
              Math.round(value) === preset
                ? "bg-emerald-600 text-white"
                : "bg-white/6 text-white/75 hover:bg-white/10"
            }`}
          >
            {preset}
          </button>
        ))}
      </div>
    </div>
  );
}

function PoseRow({ index, pose }: { index: number; pose: number[] }) {
  return (
    <div className="rounded-lg bg-black/15 px-2.5 py-2 text-[11px] text-white/70">
      <div className="mb-1 font-medium text-white/85">Diem {index + 1}</div>
      <div className="break-words leading-5">
        {pose.map((value, i) => `J${i + 1}:${Math.round(value)} deg`).join(" | ")}
      </div>
    </div>
  );
}

export default function ControlTab({
  actionLocked,
  controlsLocked,
  connected,
  joints,
  controlMode,
  jointSpeed,
  setJointSpeed,
  jointAccel,
  setJointAccel,
  jogStep,
  setJogStep,
  cycleName,
  setCycleName,
  saveError,
  saveSuccess,
  recordingActive,
  draftPoints,
  cycleViewId,
  cycles,
  queueIds,
  queueSelectMode,
  queuedCycles,
  focusDraftMode,
  poseSummary,
  lastCommandLabel,
  logs,
  onNudgeJoint,
  onJointChange,
  onHome,
  onSendPose,
  onStart,
  onStop,
  onResetDraft,
  onStartRecording,
  onSaveCurrentPoint,
  onStopAndSaveCycle,
  onRunCycle,
  onPreviewCycle,
  onDeleteCycle,
  onToggleQueue,
  onRunQueue,
  onToggleQueueMode,
  onClearSaveNotice,
}: ControlTabProps) {
  return (
    <main className="grid min-h-0 flex-1 gap-3 xl:grid-cols-[minmax(0,1.78fr)_380px] xl:overflow-hidden">
      <div className="grid min-h-0 gap-3 overflow-hidden xl:grid-rows-[auto_minmax(0,1fr)]">
        <Card title="Cum dieu khien khop">
          <div className="mb-3 grid gap-3 rounded-2xl border border-white/6 bg-white/4 p-3 xl:grid-cols-[minmax(0,1fr)_minmax(0,1fr)_auto]">
            <div>
              <div className="mb-1 text-[11px] text-white/50">Toc do dieu khien</div>
              <div className="flex items-center gap-3">
                <input
                  type="range"
                  min={5}
                  max={120}
                  value={jointSpeed}
                  disabled={actionLocked}
                  onChange={(event) => setJointSpeed(Number(event.target.value))}
                  className="w-full accent-emerald-500 disabled:opacity-40"
                />
                <div className="w-14 text-right text-sm text-white/80">{jointSpeed}</div>
              </div>
            </div>
            <div>
              <div className="mb-1 text-[11px] text-white/50">Gia toc</div>
              <div className="flex items-center gap-3">
                <input
                  type="range"
                  min={10}
                  max={100}
                  value={jointAccel}
                  disabled={actionLocked}
                  onChange={(event) => setJointAccel(Number(event.target.value))}
                  className="w-full accent-emerald-500 disabled:opacity-40"
                />
                <div className="w-14 text-right text-sm text-white/80">{jointAccel}%</div>
              </div>
            </div>
            <div>
              <div className="mb-1 text-[11px] text-white/50">Buoc vi chinh</div>
              <div className="flex gap-2">
                {[1, 5, 10].map((step) => (
                  <button
                    key={step}
                    disabled={actionLocked}
                    onClick={() => setJogStep(step)}
                    className={`rounded-xl px-3 py-1.5 text-sm transition disabled:cursor-not-allowed disabled:opacity-40 ${
                      jogStep === step
                        ? "bg-emerald-600 text-white"
                        : "bg-white/6 text-white/75 hover:bg-white/10"
                    }`}
                  >
                    {step}
                  </button>
                ))}
              </div>
            </div>
          </div>

          <div className="mb-3 grid gap-3 md:grid-cols-2">
            {[0, 1, 2, 3].map((index) => (
              <AxisCard
                key={index}
                title={`Joint ${index + 1}`}
                value={joints[index]}
                step={jogStep}
                disabled={actionLocked}
                onNudge={(delta) => onNudgeJoint(index, delta)}
                onChange={(value) => onJointChange(index, value)}
              />
            ))}
          </div>

          <div className="grid gap-3 md:grid-cols-2">
            <ServoCard
              title="Servo J5"
              hint="Co tay xoay - dung preset va vi chinh nho"
              icon={RotateCw}
              value={joints[4]}
              disabled={actionLocked}
              onChange={(value) => onJointChange(4, value)}
              presets={[0, 45, 90, 135]}
            />
            <ServoCard
              title="Servo J6"
              hint="Kep - mo, giu, dong nhanh bang preset"
              icon={Grip}
              value={joints[5]}
              disabled={actionLocked}
              onChange={(value) => onJointChange(5, value)}
              presets={[0, 30, 90, 180]}
            />
          </div>
        </Card>

        <div className="grid min-h-0 gap-3 xl:grid-cols-[minmax(0,1.05fr)_minmax(0,0.95fr)] xl:overflow-hidden">
          <Card title="Lenh thao tac va trang thai" className="h-full min-h-0 overflow-hidden">
            <div className="grid gap-2 sm:grid-cols-2 xl:grid-cols-4">
              <button
                disabled={actionLocked}
                onClick={onHome}
                className="rounded-xl bg-white/6 px-4 py-2 text-sm text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
              >
                <Home className="mr-2 inline h-4 w-4" />Home
              </button>
              <button
                disabled={actionLocked}
                onClick={onSendPose}
                className="rounded-xl bg-white/6 px-4 py-2 text-sm text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
              >
                <Save className="mr-2 inline h-4 w-4" />Gui pose
              </button>
              <button
                onClick={onStart}
                className="rounded-xl bg-emerald-600 px-4 py-2 text-sm font-medium text-white hover:bg-emerald-500"
              >
                <Play className="mr-2 inline h-4 w-4" />{controlsLocked ? "Mo va Start" : "Start"}
              </button>
              <button
                onClick={onStop}
                className="rounded-xl bg-red-600 px-4 py-2 text-sm font-medium text-white hover:bg-red-500"
              >
                <Square className="mr-2 inline h-4 w-4" />Stop
              </button>
            </div>

            <div className="mt-4 border-t border-white/8 pt-3 text-sm">
              <div className="flex flex-wrap items-center gap-x-2 gap-y-1">
                <span className="text-white/50">Che do:</span>
                <span className="font-medium text-white">
                  {controlMode === "live" ? "Live gui ngay" : "Batch gui thu cong"}
                </span>
              </div>
              <div className="mt-2 flex flex-wrap items-center gap-x-2 gap-y-1">
                <span className="text-white/50">Tom tat goc:</span>
                <span className="break-words text-white/90">{poseSummary}</span>
              </div>
              <div className="mt-2 flex flex-wrap items-center gap-x-2 gap-y-1">
                <span className="text-white/50">Lenh gan nhat:</span>
                <span className="text-white/90">{lastCommandLabel}</span>
              </div>
              {controlsLocked && (
                <div className="mt-3 rounded-xl border border-red-400/15 bg-red-500/10 px-3 py-2 text-xs text-red-300">
                  Dieu khien dang bi khoa sau khi Stop. Nhan Start de mo lai.
                </div>
              )}
            </div>
          </Card>

          <Card title="Log dieu khien" className="flex h-full min-h-0 min-w-0 flex-col overflow-hidden">
            <div
              className="min-h-0 flex-1 space-y-2 overflow-y-auto pr-1"
              style={{ scrollbarWidth: "thin", overscrollBehavior: "contain" }}
            >
              {logs.length === 0 ? (
                <div className="flex h-full min-h-[120px] items-center justify-center rounded-xl border border-dashed border-white/10 text-sm text-white/35">
                  Chua gui du lieu toi ESP32
                </div>
              ) : (
                logs.map((item, index) => (
                  <div key={index} className="rounded-xl bg-white/4 px-3 py-2.5">
                    <div className="font-mono text-[11px] text-emerald-300">{item.time}</div>
                    <div className="mt-1 break-all font-mono text-[12px] leading-5 text-white/88">
                      {item.payload}
                    </div>
                  </div>
                ))
              )}
            </div>
          </Card>
        </div>
      </div>

      <aside className="grid min-h-0 gap-3 xl:grid-rows-[minmax(0,1fr)_auto] xl:overflow-hidden">
        <Card title="Luu chu trinh" className="flex h-full min-h-0 flex-col overflow-hidden">
          <div className="flex-shrink-0">
            <div className="flex items-center gap-2">
              <input
                type="text"
                value={cycleName}
                disabled={actionLocked}
                onChange={(event) => {
                  setCycleName(event.target.value);
                  if (saveError || saveSuccess) onClearSaveNotice();
                }}
                placeholder="Ten chu trinh"
                className="flex-1 rounded-xl border border-white/15 bg-transparent px-3 py-2 text-sm text-white outline-none placeholder:text-white/40 focus:border-emerald-400/40 disabled:cursor-not-allowed disabled:opacity-40"
              />
            </div>

            <div className="mt-3 grid grid-cols-3 gap-2">
              <button
                disabled={actionLocked}
                onClick={onStartRecording}
                className={`rounded-xl px-3 py-2 text-xs font-medium disabled:cursor-not-allowed disabled:opacity-40 ${
                  recordingActive
                    ? "bg-emerald-600 text-white"
                    : "bg-white/6 text-white hover:bg-white/10"
                }`}
              >
                Bat dau luu
              </button>
              <button
                disabled={actionLocked}
                onClick={onSaveCurrentPoint}
                className="rounded-xl bg-white/6 px-3 py-2 text-xs font-medium text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
              >
                Luu diem
              </button>
              <button
                disabled={actionLocked}
                onClick={onStopAndSaveCycle}
                className="rounded-xl bg-white/6 px-3 py-2 text-xs font-medium text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
              >
                Dung va luu
              </button>
            </div>

            <div className="mt-3 flex flex-wrap items-center gap-2 text-xs">
              <span
                className={`rounded-full px-2.5 py-1 ${
                  recordingActive ? "bg-emerald-500/10 text-emerald-300" : "bg-white/6 text-white/55"
                }`}
              >
                {recordingActive ? "Dang luu" : "Chua luu"}
              </span>
              <span className="text-white/55">
                So diem: <span className="font-medium text-white">{draftPoints.length}</span>
              </span>
              {cycleName.trim() ? (
                <span className="truncate text-white/55">
                  Ten: <span className="text-white/85">{cycleName.trim()}</span>
                </span>
              ) : null}
            </div>

            {(saveError || saveSuccess) && (
              <div
                className={`mt-3 rounded-xl px-3 py-2 text-sm ${
                  saveError
                    ? "border border-red-400/20 bg-red-500/10 text-red-300"
                    : "border border-emerald-400/20 bg-emerald-500/10 text-emerald-300"
                }`}
              >
                {saveError || saveSuccess}
              </div>
            )}

            {focusDraftMode && (
              <div className="mt-3 rounded-xl border border-white/8 bg-white/4 p-3">
                <div className="mb-2 flex items-center justify-between gap-2">
                  <div className="text-sm font-medium text-white">Nhap dang luu</div>
                  <button
                    disabled={actionLocked}
                    onClick={onResetDraft}
                    className="rounded-lg bg-white/6 px-2.5 py-1 text-xs text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
                  >
                    Huy nhap
                  </button>
                </div>
                <div className="max-h-[220px] space-y-2 overflow-y-auto pr-1" style={{ scrollbarWidth: "thin" }}>
                  {draftPoints.length === 0 ? (
                    <div className="rounded-lg bg-black/15 px-3 py-2 text-[11px] text-white/45">
                      Chua co diem nao trong nhap.
                    </div>
                  ) : (
                    draftPoints.map((pose, index) => <PoseRow key={`draft-${index}`} index={index} pose={pose} />)
                  )}
                </div>
              </div>
            )}
          </div>

          {!focusDraftMode && (
            <>
              <div className="mt-4 flex-shrink-0 border-t border-white/8 pt-3">
                <div className="flex items-center justify-between gap-2">
                  <div className="text-sm font-medium text-white">Chu trinh da luu</div>
                  <div className="flex items-center gap-2">
                    {queueSelectMode ? (
                      <>
                        <button
                          disabled={actionLocked || queuedCycles.length === 0}
                          onClick={() => onRunQueue(queuedCycles)}
                          className="rounded-xl bg-emerald-600 px-3 py-2 text-xs text-white hover:bg-emerald-500 disabled:cursor-not-allowed disabled:opacity-40"
                        >
                          Run chon
                        </button>
                        <button
                          disabled={actionLocked}
                          onClick={onToggleQueueMode}
                          className="rounded-xl bg-white/6 px-3 py-2 text-xs text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
                        >
                          Xong
                        </button>
                      </>
                    ) : (
                      <>
                        <button
                          disabled={actionLocked || cycles.length === 0}
                          onClick={() => onRunQueue(cycles)}
                          className="rounded-xl bg-emerald-600 px-3 py-2 text-xs text-white hover:bg-emerald-500 disabled:cursor-not-allowed disabled:opacity-40"
                        >
                          Run All
                        </button>
                        <button
                          disabled={actionLocked || cycles.length === 0}
                          onClick={onToggleQueueMode}
                          className="rounded-xl bg-white/6 px-3 py-2 text-xs text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
                        >
                          Chon hang
                        </button>
                      </>
                    )}
                  </div>
                </div>
              </div>

              <div
                className="mt-3 min-h-0 flex-1 space-y-2 overflow-y-auto pr-1"
                style={{ scrollbarWidth: "thin", overscrollBehavior: "contain" }}
              >
                {cycles.length === 0 ? (
                  <div className="flex h-full min-h-[140px] items-center justify-center rounded-xl border border-dashed border-white/10 text-sm text-white/35">
                    Chua co chu trinh nao
                  </div>
                ) : (
                  cycles.map((cycle) => {
                    const checked = queueIds.includes(cycle.id);
                    return (
                      <div key={cycle.id} className="rounded-xl bg-white/5 px-3 py-3">
                        <div className="flex items-center gap-3">
                          {queueSelectMode && (
                            <button
                              disabled={actionLocked}
                              onClick={() => onToggleQueue(cycle.id)}
                              className={`h-5 w-5 flex-shrink-0 rounded border transition disabled:cursor-not-allowed disabled:opacity-40 ${
                                checked
                                  ? "border-emerald-400 bg-emerald-500"
                                  : "border-white/20 bg-transparent hover:border-white/40"
                              }`}
                              aria-label={`Chon ${cycle.name}`}
                            />
                          )}

                          <div className="min-w-0 flex-1">
                            <div className="flex min-w-0 items-center justify-between gap-3">
                              <div className="min-w-0 flex-1">
                                <div className="truncate text-sm font-semibold text-white">{cycle.name}</div>
                                <div className="mt-1 text-[11px] text-white/45">
                                  {cycle.points.length} diem | {cycle.speed} deg/s | {cycle.createdAt}
                                </div>
                              </div>

                              <div className="flex items-center gap-1.5">
                                <button
                                  disabled={actionLocked}
                                  onClick={() => onPreviewCycle(cycle.id)}
                                  className="rounded-lg bg-white/6 px-2.5 py-1.5 text-xs text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
                                >
                                  <Eye className="mr-1 inline h-3.5 w-3.5" />Xem
                                </button>
                                <button
                                  disabled={actionLocked}
                                  onClick={() => onRunCycle(cycle)}
                                  className="rounded-lg bg-emerald-600 px-2.5 py-1.5 text-xs text-white hover:bg-emerald-500 disabled:cursor-not-allowed disabled:opacity-40"
                                >
                                  <Play className="mr-1 inline h-3.5 w-3.5" />Run
                                </button>
                                <button
                                  disabled={actionLocked}
                                  onClick={() => onDeleteCycle(cycle.id)}
                                  className="rounded-lg bg-red-600 px-2.5 py-1.5 text-xs text-white hover:bg-red-500 disabled:cursor-not-allowed disabled:opacity-40"
                                >
                                  <Trash2 className="mr-1 inline h-3.5 w-3.5" />Xoa
                                </button>
                              </div>
                            </div>
                          </div>
                        </div>

                        {cycleViewId === cycle.id && (
                          <div className="mt-3 space-y-2 border-t border-white/6 pt-3">
                            {cycle.points.map((pose, index) => (
                              <PoseRow key={`${cycle.id}-${index}`} index={index} pose={pose} />
                            ))}
                          </div>
                        )}
                      </div>
                    );
                  })
                )}
              </div>
            </>
          )}
        </Card>

        <Card title="Dieu khien tong quan" className="overflow-hidden">
          <div className="grid grid-cols-2 gap-2">
            <TinyStat
              icon={connected ? Wifi : WifiOff}
              label="Ket noi"
              value={connected ? "Online" : "Offline"}
              tone={connected ? "good" : "warn"}
            />
            <TinyStat icon={Zap} label="Toc do" value={`${jointSpeed} deg/s`} />
            <TinyStat icon={Settings2} label="Gia toc" value={`${jointAccel}%`} />
            <TinyStat icon={CircleDot} label="Buoc jog" value={`+/- ${jogStep}`} />
          </div>
        </Card>
      </aside>
    </main>
  );
}
