import { useCallback, useEffect, useRef, useState } from "react";

// Kết nối WebSocket thật tới ESP32 Wroom (endpoint "/ws", xem
// firmware/ESP32_Wroom_Main/LinkWeb.h). Giao thức JSON giữ nguyên các gói
// tin mà robot/main.tsx đã xây dựng sẵn (cmd: J/HOME/START/ESTOP/PING/
// RUN_CYCLE/RUN_QUEUE); ESP32 phát lại trạng thái định kỳ dạng
// {type:"status", state, p:[j1..j6], moving, fault}.

export type EspStatus = {
  state: string;
  p: number[];
  moving: boolean;
  fault: number;
};

const RECONNECT_DELAY_MS = 2000;

export function useEspLink() {
  const [connected, setConnected] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [lastStatus, setLastStatus] = useState<EspStatus | null>(null);

  const wsRef = useRef<WebSocket | null>(null);
  const shouldReconnectRef = useRef(false);
  const reconnectTimerRef = useRef<number | null>(null);
  const targetRef = useRef<{ ip: string; port: string } | null>(null);

  const clearReconnectTimer = () => {
    if (reconnectTimerRef.current !== null) {
      window.clearTimeout(reconnectTimerRef.current);
      reconnectTimerRef.current = null;
    }
  };

  const openSocket = useCallback((ip: string, port: string) => {
    targetRef.current = { ip, port };
    setConnecting(true);

    const ws = new WebSocket(`ws://${ip}:${port}/ws`);
    wsRef.current = ws;

    ws.onopen = () => {
      setConnected(true);
      setConnecting(false);
    };

    ws.onclose = () => {
      setConnected(false);
      setConnecting(false);
      wsRef.current = null;
      // Tu ket noi lai neu chua bi nguoi dung chu dong Ngat — quan trong
      // cho "thoi gian thuc": mat song WiFi thoang qua khong can bam lai.
      if (shouldReconnectRef.current && targetRef.current) {
        clearReconnectTimer();
        reconnectTimerRef.current = window.setTimeout(() => {
          if (shouldReconnectRef.current && targetRef.current) {
            openSocket(targetRef.current.ip, targetRef.current.port);
          }
        }, RECONNECT_DELAY_MS);
      }
    };

    ws.onerror = () => {
      ws.close();
    };

    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        if (data && data.type === "status") {
          setLastStatus({
            state: typeof data.state === "string" ? data.state : "IDLE",
            p: Array.isArray(data.p) ? data.p : [],
            moving: Boolean(data.moving),
            fault: Number(data.fault ?? 0),
          });
        }
      } catch {
        // bo qua ban tin khong phai JSON hop le
      }
    };
  }, []);

  const connect = useCallback(
    (ip: string, port: string) => {
      shouldReconnectRef.current = true;
      clearReconnectTimer();
      wsRef.current?.close();
      openSocket(ip, port);
    },
    [openSocket]
  );

  const disconnect = useCallback(() => {
    shouldReconnectRef.current = false;
    clearReconnectTimer();
    wsRef.current?.close();
    wsRef.current = null;
    setConnected(false);
    setConnecting(false);
  }, []);

  const send = useCallback((payload: unknown) => {
    const ws = wsRef.current;
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(payload));
      return true;
    }
    return false;
  }, []);

  useEffect(() => {
    return () => {
      shouldReconnectRef.current = false;
      clearReconnectTimer();
      wsRef.current?.close();
    };
  }, []);

  return { connected, connecting, lastStatus, connect, disconnect, send };
}
