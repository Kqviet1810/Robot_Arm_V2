import { useEffect, useMemo, useState } from "react";
import type { ReactNode } from "react";
import { Wifi, WifiOff } from "lucide-react";
import ControlTab, { type Mode, type PayloadLog, type SavedCycle } from "./control";
import HomeTab from "./home";
import SettingTab, { type GoHomeMode } from "./setting";
import { useEspLink } from "./espLink";

type TabKey = "home" | "control" | "settings";

const clamp = (v: number, min: number, max: number) => Math.max(min, Math.min(max, v));
const normalizeName = (name: string) => name.trim().replace(/\s+/g, " ").toLowerCase();
const roundPose = (pose: number[]) => pose.map((v) => Math.round(v));
const timeNow = () =>
  new Date().toLocaleTimeString("vi-VN", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });

function TabButton({
  active,
  onClick,
  children,
}: {
  active: boolean;
  onClick: () => void;
  children: ReactNode;
}) {
  return (
    <button
      onClick={onClick}
      className={`rounded-xl px-3 py-1.5 text-sm transition ${
        active ? "bg-emerald-600 text-white" : "bg-white/6 text-white/75 hover:bg-white/10"
      }`}
    >
      {children}
    </button>
  );
}

export default function RobotControlMain() {
  const [activeTab, setActiveTab] = useState<TabKey>("control");
  const espLink = useEspLink();
  const connected = espLink.connected;
  const [ip, setIp] = useState("192.168.4.1");
  const [port] = useState("80");

  const [joints, setJoints] = useState<number[]>([82, -9, 0, -34, 90, 66]);
  const controlMode: Mode = "live";
  const [jointSpeed, setJointSpeed] = useState(45);
  const [jointAccel, setJointAccel] = useState(60);
  const [jogStep, setJogStep] = useState(5);
  const [controlsLocked, setControlsLocked] = useState(false);

  const [goHomeMode, setGoHomeMode] = useState<GoHomeMode>("esp_default");
  const [goHomeSpeed, setGoHomeSpeed] = useState(35);
  const [goHomeSmooth, setGoHomeSmooth] = useState(true);

  const [cycles, setCycles] = useState<SavedCycle[]>(() => {
    try {
      const raw = localStorage.getItem("robot_cycles_production_v3");
      const parsed = raw ? JSON.parse(raw) : [];
      return Array.isArray(parsed) ? parsed : [];
    } catch {
      return [];
    }
  });
  const [cycleName, setCycleName] = useState("");
  const [recordingActive, setRecordingActive] = useState(false);
  const [draftPoints, setDraftPoints] = useState<number[][]>([]);
  const [cycleViewId, setCycleViewId] = useState<number | null>(null);
  const [queueIds, setQueueIds] = useState<number[]>([]);
  const [queueSelectMode, setQueueSelectMode] = useState(false);
  const [saveError, setSaveError] = useState("");
  const [saveSuccess, setSaveSuccess] = useState("");

  const [lastCommandLabel, setLastCommandLabel] = useState("Chua gui lenh");
  const [logs, setLogs] = useState<PayloadLog[]>([]);

  useEffect(() => {
    localStorage.setItem("robot_cycles_production_v3", JSON.stringify(cycles));
  }, [cycles]);

  // Cap nhat pose tren UI theo trang thai that tu ESP32 (quan trong khi
  // robot tu chay HOME/RUN_CYCLE/RUN_QUEUE, khong phai do keo slider).
  useEffect(() => {
    if (espLink.lastStatus && espLink.lastStatus.p.length === 6) {
      setJoints(espLink.lastStatus.p);
    }
  }, [espLink.lastStatus]);

  const poseSummary = useMemo(
    () => joints.map((v, i) => `J${i + 1}:${Math.round(v)} deg`).join(" | "),
    [joints]
  );

  const queuedCycles = useMemo(
    () => queueIds.map((id) => cycles.find((cycle) => cycle.id === id)).filter(Boolean) as SavedCycle[],
    [queueIds, cycles]
  );

  const focusDraftMode = recordingActive || draftPoints.length > 0;
  const actionLocked = controlsLocked;

  function writeEspPacket(payload: Record<string, unknown>, label: string) {
    const packet = JSON.stringify(payload);
    const now = timeNow();

    setLastCommandLabel(label);
    setLogs((prev) => [{ time: now, payload: packet }, ...prev].slice(0, 80));

    espLink.send(payload);
  }

  function buildPosePacket(pose: number[], mode: "live" | "manual") {
    return {
      cmd: "J",
      p: roundPose(pose),
      v: jointSpeed,
      a: jointAccel,
      m: mode,
    };
  }

  function buildCyclePacket(cycle: SavedCycle) {
    return {
      cmd: "RUN_CYCLE",
      name: cycle.name,
      pts: cycle.points.map((point) => roundPose(point)),
      v: cycle.speed,
      gh: {
        mode: goHomeMode,
        v: goHomeSpeed,
        smooth: goHomeSmooth ? 1 : 0,
      },
    };
  }

  function buildQueuePacket(items: SavedCycle[]) {
    return {
      cmd: "RUN_QUEUE",
      items: items.map((cycle, index) => ({
        order: index + 1,
        name: cycle.name,
        pts: cycle.points.map((point) => roundPose(point)),
        v: cycle.speed,
      })),
      gh: {
        mode: goHomeMode,
        v: goHomeSpeed,
        smooth: goHomeSmooth ? 1 : 0,
      },
    };
  }

  function updateJoint(index: number, value: number) {
    setJoints((prev) => {
      const next = [...prev];
      next[index] = value;
      return next;
    });
  }

  function handleJointChange(index: number, value: number) {
    if (actionLocked) return;

    const min = index >= 4 ? 0 : -180;
    const max = 180;
    const safeValue = clamp(value, min, max);
    const pose = [...joints];
    pose[index] = safeValue;
    updateJoint(index, safeValue);

    if (controlMode === "live" && connected) {
      writeEspPacket(buildPosePacket(pose, "live"), `Live J${index + 1} -> ${Math.round(safeValue)} deg`);
    }
  }

  function nudgeJoint(index: number, delta: number) {
    if (actionLocked) return;
    const min = index >= 4 ? 0 : -180;
    const max = 180;
    handleJointChange(index, clamp(joints[index] + delta, min, max));
  }

  function handleHome() {
    if (actionLocked) return;
    const pose = [0, 0, 0, 0, 90, 90];
    setJoints(pose);
    if (connected) writeEspPacket({ cmd: "HOME", p: pose }, "Home chuan");
  }

  function handleSendPose() {
    if (actionLocked) return;
    if (connected) writeEspPacket(buildPosePacket(joints, "manual"), "Gui pose hien tai");
  }

  function handleStart() {
    if (controlsLocked) setControlsLocked(false);
    if (connected) {
      writeEspPacket(
        {
          cmd: "START",
          p: roundPose(joints),
          v: jointSpeed,
          a: jointAccel,
          m: controlMode,
        },
        controlsLocked ? "Mo dieu khien va Start" : `Start @ ${jointSpeed} deg/s`
      );
    }
  }

  function handleStop() {
    writeEspPacket({ cmd: "ESTOP" }, "Stop khan");
    setControlsLocked(true);
    setRecordingActive(false);
    setQueueSelectMode(false);
  }

  function resetDraft() {
    if (actionLocked) return;
    setRecordingActive(false);
    setDraftPoints([]);
    setSaveError("");
    setSaveSuccess("");
  }

  function startRecording() {
    if (actionLocked) return;

    if (recordingActive) {
      setSaveError("Dang trong che do luu. Hay luu diem hoac dung luu truoc.");
      setSaveSuccess("");
      return;
    }

    setRecordingActive(true);
    setDraftPoints([]);
    setSaveError("");
    setSaveSuccess("Da bat dau luu chu trinh.");
  }

  function saveCurrentPoint() {
    if (actionLocked) return;

    if (!recordingActive) {
      setSaveError("Hay bam Bat dau luu truoc khi luu diem.");
      setSaveSuccess("");
      return;
    }

    const pointIndex = draftPoints.length + 1;
    setDraftPoints((prev) => [...prev, [...joints]]);
    setSaveError("");
    setSaveSuccess(`Da luu diem ${pointIndex}.`);
  }

  function stopAndSaveCycle() {
    if (actionLocked) return;

    const rawName = cycleName.trim();
    const normalized = normalizeName(rawName);

    setSaveError("");
    setSaveSuccess("");

    if (!recordingActive) {
      setSaveError("Hay bam Bat dau luu truoc.");
      return;
    }

    if (!rawName) {
      setSaveError("Hay nhap ten chu trinh.");
      return;
    }

    if (draftPoints.length < 2) {
      setSaveError("Chu trinh can it nhat 2 diem da luu.");
      return;
    }

    const existed = cycles.some((item) => normalizeName(item.name) === normalized);
    if (existed) {
      setSaveError("Ten chu trinh da ton tai. Hay doi ten khac.");
      return;
    }

    const item: SavedCycle = {
      id: Date.now(),
      name: rawName,
      points: draftPoints.map((point) => [...point]),
      speed: jointSpeed,
      createdAt: new Date().toLocaleTimeString("vi-VN", {
        hour: "2-digit",
        minute: "2-digit",
      }),
    };

    setCycles((prev) => [item, ...prev]);
    setRecordingActive(false);
    setDraftPoints([]);
    setCycleName("");
    setSaveSuccess(`Da luu chu trinh "${rawName}" voi ${item.points.length} diem.`);
  }

  function runCycle(cycle: SavedCycle) {
    if (actionLocked) return;

    const lastPose = cycle.points[cycle.points.length - 1];
    if (lastPose) setJoints([...lastPose]);
    writeEspPacket(buildCyclePacket(cycle), `Chay chu trinh ${cycle.name}`);
  }

  function previewCycle(id: number) {
    if (actionLocked) return;
    setCycleViewId((prev) => (prev === id ? null : id));
  }

  function deleteCycle(id: number) {
    if (actionLocked) return;
    setCycles((prev) => prev.filter((item) => item.id !== id));
    setQueueIds((prev) => prev.filter((x) => x !== id));
    if (cycleViewId === id) setCycleViewId(null);
  }

  function toggleQueue(id: number) {
    if (actionLocked || !queueSelectMode) return;
    setQueueIds((prev) => (prev.includes(id) ? prev.filter((x) => x !== id) : [...prev, id]));
  }

  function runQueue(items: SavedCycle[]) {
    if (actionLocked || items.length === 0) return;
    writeEspPacket(buildQueuePacket(items), `Chay hang chu trinh (${items.length})`);
  }

  function toggleQueueMode() {
    if (actionLocked) return;
    setQueueSelectMode((prev) => {
      const next = !prev;
      if (!next) setQueueIds([]);
      return next;
    });
  }

  return (
    <div className="h-screen overflow-hidden bg-[#081228] text-white">
      <div className="mx-auto flex h-full max-w-[1600px] flex-col gap-3 p-3">
        <header className="rounded-2xl border border-white/8 bg-[#0d1830]/90 px-4 py-2.5 shadow-2xl shadow-black/20">
          <div className="flex flex-wrap items-center justify-between gap-3">
            <div className="flex flex-wrap items-center gap-4">
              <div className="text-lg font-semibold">Bang dieu khien robot arm</div>
              <div
                className={`flex items-center gap-2 rounded-full px-3 py-1 text-sm ${
                  connected ? "bg-emerald-500/10 text-emerald-300" : "bg-red-500/10 text-red-300"
                }`}
              >
                {connected ? <Wifi className="h-4 w-4" /> : <WifiOff className="h-4 w-4" />}
                {connected ? "ESP32: Da ket noi" : espLink.connecting ? "ESP32: Dang ket noi..." : "ESP32: Ngat ket noi"}
              </div>
              <div className="text-sm text-white/60">
                IP: {ip} | Port: {port}
              </div>
            </div>

            <div className="flex items-center gap-2">
              <button
                disabled={!connected}
                onClick={() => writeEspPacket({ cmd: "PING" }, "Ping ESP32")}
                className="rounded-xl bg-white/6 px-3 py-2 text-sm text-white hover:bg-white/10 disabled:cursor-not-allowed disabled:opacity-40"
              >
                Ping
              </button>
              <button
                onClick={() => (connected ? espLink.disconnect() : espLink.connect(ip, port))}
                className="rounded-xl bg-white/6 px-3 py-2 text-sm text-white hover:bg-white/10"
              >
                {connected ? "Ngat" : "Ket noi"}
              </button>
            </div>
          </div>

          <div className="mt-3 flex flex-wrap items-center gap-2">
            <TabButton active={activeTab === "home"} onClick={() => setActiveTab("home")}>
              Trang chu
            </TabButton>
            <TabButton active={activeTab === "control"} onClick={() => setActiveTab("control")}>
              Dieu khien
            </TabButton>
            <TabButton active={activeTab === "settings"} onClick={() => setActiveTab("settings")}>
              Cai dat
            </TabButton>
          </div>
        </header>

        {activeTab === "home" && (
          <HomeTab connected={connected} jointSpeed={jointSpeed} poseSummary={poseSummary} />
        )}

        {activeTab === "settings" && (
          <SettingTab
            ip={ip}
            setIp={setIp}
            port={port}
            goHomeMode={goHomeMode}
            setGoHomeMode={setGoHomeMode}
            goHomeSpeed={goHomeSpeed}
            setGoHomeSpeed={setGoHomeSpeed}
            goHomeSmooth={goHomeSmooth}
            setGoHomeSmooth={setGoHomeSmooth}
          />
        )}

        {activeTab === "control" && (
          <ControlTab
            actionLocked={actionLocked}
            controlsLocked={controlsLocked}
            connected={connected}
            joints={joints}
            controlMode={controlMode}
            jointSpeed={jointSpeed}
            setJointSpeed={setJointSpeed}
            jointAccel={jointAccel}
            setJointAccel={setJointAccel}
            jogStep={jogStep}
            setJogStep={setJogStep}
            cycleName={cycleName}
            setCycleName={setCycleName}
            saveError={saveError}
            saveSuccess={saveSuccess}
            recordingActive={recordingActive}
            draftPoints={draftPoints}
            cycleViewId={cycleViewId}
            cycles={cycles}
            queueIds={queueIds}
            queueSelectMode={queueSelectMode}
            queuedCycles={queuedCycles}
            focusDraftMode={focusDraftMode}
            poseSummary={poseSummary}
            lastCommandLabel={lastCommandLabel}
            logs={logs}
            onNudgeJoint={nudgeJoint}
            onJointChange={handleJointChange}
            onHome={handleHome}
            onSendPose={handleSendPose}
            onStart={handleStart}
            onStop={handleStop}
            onResetDraft={resetDraft}
            onStartRecording={startRecording}
            onSaveCurrentPoint={saveCurrentPoint}
            onStopAndSaveCycle={stopAndSaveCycle}
            onRunCycle={runCycle}
            onPreviewCycle={previewCycle}
            onDeleteCycle={deleteCycle}
            onToggleQueue={toggleQueue}
            onRunQueue={runQueue}
            onToggleQueueMode={toggleQueueMode}
            onClearSaveNotice={() => {
              setSaveError("");
              setSaveSuccess("");
            }}
          />
        )}
      </div>
    </div>
  );
}
