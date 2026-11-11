#!/usr/bin/env node

/**
 * Herdr ESP8266 WiFi WebSocket Bridge
 * Servidor WebSocket para macOS que lee el estado de Herdr en tiempo real
 * desde su Unix Socket local y lo transmite a pantallas ESP8266 por red local WiFi.
 */

import net from 'node:net';
import path from 'node:path';
import os from 'node:os';
import { WebSocketServer, WebSocket } from 'ws';

const HERDR_SOCK_PATH = process.env.HERDR_SOCKET_PATH || path.join(os.homedir(), '.config/herdr/herdr.sock');
const PORT = parseInt(process.env.HERDR_PORT || '8266', 10);
const SNAPSHOT_INTERVAL_MS = 250;
const HEARTBEAT_INTERVAL_MS = 2000;

let lastPayloadSent = '';
let lastSentTime = 0;
const connectedClients = new Set();

// Obtener IPs locales de la Mac
function getLocalIpAddresses() {
  const interfaces = os.networkInterfaces();
  const ips = [];
  for (const name of Object.keys(interfaces)) {
    for (const iface of interfaces[name]) {
      if (iface.family === 'IPv4' && !iface.internal) {
        ips.push(iface.address);
      }
    }
  }
  return ips;
}

const localIps = getLocalIpAddresses();

console.log('========================================================');
console.log('  Herdr ESP8266 WiFi WebSocket Server');
console.log('========================================================');
console.log(`[Bridge] Socket de Herdr : ${HERDR_SOCK_PATH}`);
console.log(`[Bridge] Puerto WebSocket: ${PORT}`);
console.log(`[Bridge] IP(s) local(es) para secrets.h:`);
localIps.forEach((ip) => console.log(`   ➔ ws://${ip}:${PORT}/ws`));
console.log('========================================================');

// ==========================================
// 1. INICIAR SERVIDOR WEBSOCKET
// ==========================================
const wss = new WebSocketServer({ port: PORT, path: '/ws' });

wss.on('connection', (ws, req) => {
  const clientIp = req.socket.remoteAddress;
  console.log(`[Bridge] Nuevo ESP8266 conectado desde ${clientIp}`);
  connectedClients.add(ws);

  // Enviar de inmediato el último estado disponible
  if (lastPayloadSent) {
    ws.send(lastPayloadSent);
  }

  ws.on('message', (message) => {
    try {
      const msgStr = message.toString();
      if (msgStr.includes('HERDR_DISCOVER')) {
        ws.send(JSON.stringify({ device: 'herdr-tft', v: 1, ready: true }));
      }
    } catch (e) {}
  });

  ws.on('close', () => {
    console.log(`[Bridge] ESP8266 desconectado (${clientIp})`);
    connectedClients.delete(ws);
  });

  ws.on('error', (err) => {
    console.error(`[Bridge] Error en socket cliente:`, err.message);
    connectedClients.delete(ws);
  });
});

wss.on('error', (err) => {
  console.error('[Bridge] Error en Servidor WebSocket:', err.message);
});

// ==========================================
// 2. GESTIÓN DEL SOCKET DE HERDR
// ==========================================
function fetchHerdrSnapshot() {
  return new Promise((resolve) => {
    let client;
    let settled = false;

    const finish = (result) => {
      if (settled) return;
      settled = true;
      if (client) client.destroy();
      resolve(result);
    };

    const timer = setTimeout(() => finish(null), 400);

    try {
      client = net.createConnection(HERDR_SOCK_PATH, () => {
        const req = { id: `ws_br:${Date.now()}`, method: 'session.snapshot', params: {} };
        client.write(JSON.stringify(req) + '\n');
      });

      let buffer = '';
      client.on('data', (chunk) => {
        buffer += chunk.toString();
        const newlineIdx = buffer.indexOf('\n');
        if (newlineIdx >= 0) {
          const line = buffer.slice(0, newlineIdx);
          try {
            const parsed = JSON.parse(line);
            clearTimeout(timer);
            finish(parsed?.result?.snapshot || null);
          } catch (e) {
            clearTimeout(timer);
            finish(null);
          }
        }
      });

      client.on('error', () => {
        clearTimeout(timer);
        finish(null);
      });
    } catch (e) {
      clearTimeout(timer);
      finish(null);
    }
  });
}

function parseSnapshotToDisplayData(snapshot) {
  if (!snapshot) {
    return {
      status: 'idle',
      agent: 'herdr',
      title: 'Herdr Activo',
      detail: 'Sin tareas activas',
      tabs: []
    };
  }

  const agents = snapshot.agents || [];
  const panes = snapshot.panes || [];
  const focusedPaneId = snapshot.focused_pane_id;

  let currentAgent = agents.find((a) => a.pane_id === focusedPaneId) || agents[0];

  const busyAgent = agents.find((a) => a.agent_status === 'working' || a.agent_status === 'blocked');
  if (busyAgent && currentAgent?.agent_status === 'idle') {
    currentAgent = busyAgent;
  }

  let status = 'idle';
  let detail = 'Listo';

  if (currentAgent) {
    const rawStatus = currentAgent.agent_status;
    if (rawStatus === 'working') {
      status = 'working';
      detail = 'Pensando...';
    } else if (rawStatus === 'blocked') {
      status = 'blocked';
      detail = 'Requiere permiso / input';
    } else if (rawStatus === 'idle') {
      status = 'idle';
      detail = 'Sesión lista';
    }
  }

  let title = currentAgent?.terminal_title_stripped || currentAgent?.terminal_title || 'Sesión Herdr';
  if (title.startsWith('OC | ')) {
    title = title.substring(5).trim();
  }

  const tabs = [];
  const relevantPanes = panes.filter((p) => p.agent_status && p.agent_status !== 'unknown').slice(0, 6);
  for (const p of relevantPanes) {
    let s = 'i';
    if (p.agent_status === 'working') s = 'w';
    else if (p.agent_status === 'blocked') s = 'b';
    tabs.push({
      s,
      f: p.pane_id === focusedPaneId
    });
  }

  return {
    status,
    agent: currentAgent?.agent || 'opencode',
    title: title.slice(0, 36),
    detail: detail.slice(0, 24),
    tabs
  };
}

// ==========================================
// 3. BROADCAST PERIÓDICO A LOS CLIENTES
// ==========================================
async function mainLoop() {
  if (connectedClients.size === 0) {
    return; // Si no hay pantallas conectadas por WiFi, descansar CPU
  }

  const snapshot = await fetchHerdrSnapshot();
  const data = parseSnapshotToDisplayData(snapshot);
  const jsonStr = JSON.stringify(data);
  const now = Date.now();

  if (jsonStr !== lastPayloadSent || (now - lastSentTime >= HEARTBEAT_INTERVAL_MS)) {
    lastPayloadSent = jsonStr;
    lastSentTime = now;

    for (const ws of connectedClients) {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(jsonStr);
      }
    }
  }
}

setInterval(mainLoop, SNAPSHOT_INTERVAL_MS);
