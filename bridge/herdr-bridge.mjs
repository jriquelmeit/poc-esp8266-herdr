#!/usr/bin/env node

/**
 * Herdr ESP8266 TFT Companion Bridge
 * Lee el estado en tiempo real desde el socket de Herdr y lo transmite
 * al ESP8266 vía USB Serial con handshake seguro para no interferir con otros dispositivos.
 */

import net from 'node:net';
import path from 'node:path';
import os from 'node:os';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';

// Configuración por defecto
const HERDR_SOCK_PATH = process.env.HERDR_SOCKET_PATH || path.join(os.homedir(), '.config/herdr/herdr.sock');
const BAUD_RATE = 115200;
const SNAPSHOT_INTERVAL_MS = 250;
const HEARTBEAT_INTERVAL_MS = 1500;
const PORT_SCAN_INTERVAL_MS = 2000;

let activeSerialPort = null;
let activeParser = null;
let activeDevicePath = null;
let lastPayloadSent = '';
let lastSentTime = 0;
const ignoredPorts = new Map(); // path -> timestamp hasta cuando ignorar

console.log('[Bridge] Iniciando Herdr ESP8266 Bridge...');
console.log(`[Bridge] Socket Herdr: ${HERDR_SOCK_PATH}`);

// ==========================================
// 1. GESTIÓN DEL SOCKET DE HERDR
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
        const req = { id: `br:${Date.now()}`, method: 'session.snapshot', params: {} };
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

  // Buscar agente enfocado o el primer agente activo
  let currentAgent = agents.find((a) => a.pane_id === focusedPaneId) || agents[0];

  // Si el agente enfocado está en idle pero otro está trabajando/bloqueado, darle prioridad visual
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

  // Título de la sesión o tarea
  let title = currentAgent?.terminal_title_stripped || currentAgent?.terminal_title || 'Sesión Herdr';
  // Limpiar prefijo común "OC | " para que se lea mejor en la pantalla de 128px
  if (title.startsWith('OC | ')) {
    title = title.substring(5).trim();
  }

  // Resumen de pestañas / panes (hasta 6)
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
// 2. DETECCIÓN INTELIGENTE Y HANDSHAKE SERIAL
// ==========================================
async function scanAndConnectSerial() {
  if (activeSerialPort && activeSerialPort.isOpen) {
    return;
  }

  try {
    const ports = await SerialPort.list();
    const now = Date.now();

    // Filtrar candidatos USB comunes en macOS
    const candidates = ports.filter((p) => {
      const path = p.path.toLowerCase();
      const isUsb = path.includes('usbserial') || path.includes('wchusbserial') || path.includes('usbmodem');
      const isIgnored = ignoredPorts.has(p.path) && ignoredPorts.get(p.path) > now;
      return isUsb && !isIgnored;
    });

    for (const portInfo of candidates) {
      const isHerdrTFT = await tryHandshake(portInfo.path);
      if (isHerdrTFT) {
        console.log(`[Bridge] Dispositivo Herdr TFT conectado exitosamente en ${portInfo.path}`);
        return;
      } else {
        console.log(`[Bridge] Dispositivo en ${portInfo.path} no es Herdr TFT. Ignorando 15s para no bloquear otros proyectos.`);
        ignoredPorts.set(portInfo.path, now + 15000);
      }
    }
  } catch (err) {
    console.error('[Bridge] Error escaneando puertos serie:', err.message);
  }
}

function tryHandshake(portPath) {
  return new Promise((resolve) => {
    let port;
    let parser;
    let resolved = false;

    const cleanup = () => {
      if (resolved) return;
      resolved = true;
      if (port && port.isOpen) {
        port.close(() => resolve(false));
      } else {
        resolve(false);
      }
    };

    const timeout = setTimeout(cleanup, 1200);

    try {
      port = new SerialPort({ path: portPath, baudRate: BAUD_RATE, autoOpen: false });
      parser = port.pipe(new ReadlineParser({ delimiter: '\n' }));

      port.open((err) => {
        if (err) {
          clearTimeout(timeout);
          cleanup();
          return;
        }

        // Enviar handshake de descubrimiento
        port.write('{"cmd":"HERDR_DISCOVER"}\n');

        parser.on('data', (line) => {
          line = line.trim();
          if (line.includes('herdr-tft')) {
            clearTimeout(timeout);
            resolved = true;
            activeSerialPort = port;
            activeParser = parser;
            activeDevicePath = portPath;

            // Configurar manejadores permanentes
            port.on('close', () => {
              console.log(`[Bridge] Conexión serial cerrada en ${portPath}`);
              activeSerialPort = null;
              activeParser = null;
              activeDevicePath = null;
            });
            port.on('error', (e) => {
              console.error(`[Bridge] Error en puerto ${portPath}:`, e.message);
              activeSerialPort = null;
              activeParser = null;
              activeDevicePath = null;
            });

            resolve(true);
          }
        });
      });
    } catch (e) {
      clearTimeout(timeout);
      cleanup();
    }
  });
}

// ==========================================
// 3. BUCLE PRINCIPAL DE ENVÍO
// ==========================================
async function mainLoop() {
  if (activeSerialPort && activeSerialPort.isOpen) {
    const snapshot = await fetchHerdrSnapshot();
    const data = parseSnapshotToDisplayData(snapshot);
    const jsonStr = JSON.stringify(data);
    const now = Date.now();

    // Enviar solo si cambió el estado o pasaron más de HEARTBEAT_INTERVAL_MS
    if (jsonStr !== lastPayloadSent || (now - lastSentTime >= HEARTBEAT_INTERVAL_MS)) {
      activeSerialPort.write(jsonStr + '\n', (err) => {
        if (!err) {
          lastPayloadSent = jsonStr;
          lastSentTime = now;
        }
      });
    }
  } else {
    // Si no hay puerto conectado, escanear periódicamente
    await scanAndConnectSerial();
  }
}

// Iniciar bucles periódicos
setInterval(mainLoop, SNAPSHOT_INTERVAL_MS);
console.log('[Bridge] Monitoreando puertos serie y socket Herdr...');
