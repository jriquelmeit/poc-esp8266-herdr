# Herdr ESP8266 TFT Companion (1.77" ST7735) - Rama WiFi 📡

Monitor de escritorio físico **100% Inalámbrico (WiFi)** para **Herdr** y agentes de IA (como **OpenCode**). Muestra en tiempo real lo que está haciendo la IA (pensando, ejecutando herramientas, esperando confirmación, sesión lista) con animaciones fluidas a 30-60 FPS en una pantalla TFT ST7735 de 1.77" (128x160) sobre un NodeMCU ESP8266.

Al operar por **WiFi (WebSockets)**, la pantalla solo necesita alimentación de energía (un cargador de pared o batería), liberando por completo tus puertos USB. **Cero conflictos con otros ESP32/ESP8266** que tengas conectados a tu Mac.

---

## 📋 Características

* **Conexión Inalámbrica:** Comunicación en tiempo real por WebSockets hacia el servidor local de tu Mac.
* **Cero interferencia USB:** Puedes conectar, flashear o cargar cualquier otro dispositivo por USB (como tu ESP32 de Spotify) sin que compitan por ningún puerto serie.
* **Animaciones Fluidas (60 FPS):** Motor gráfico optimizado con `TFT_eSPI` y sprites en RAM.
* **Estados en Tiempo Real:**
  * 🌀 **Procesando / Pensando (`working`):** Radar orbital neón con satélite rotatorio y partículas.
  * ⚙️ **Escribiendo / Herramienta (`tool`):** Engranaje animado con cursor parpadeante y nombre de herramienta/acción.
  * 🔔 **Requiere Atención / Pregunta (`blocked`):** Campana/alerta pulsante en ámbar.
  * ✅ **Sesión Lista (`idle`):** Círculo verde con halo armónico y checkmark.
  * 💤 **Standby / Desconectado:** Avatar durmiendo si la Mac duerme o se suspende el flujo.
* **Barra inferior multi-sesión:** Indicadores tipo píldora (dots) con el estado de cada tab/pane activo en Herdr.
* **Marquee Suave:** Títulos largos de sesión se desplazan suavemente sin cortar el texto.

---

## 🔌 Cableado y Conexión de Pines

Conexión entre el **NodeMCU ESP8266 (ESP-12E / Lolin V3)** y la pantalla **TFT 1.77" ST7735 (128x160)**:

| Pin Pantalla TFT | Pin NodeMCU (ESP8266) | GPIO Hardware | Descripción |
| :--- | :--- | :--- | :--- |
| **VCC** | **3V3** (o 5V si tu módulo tiene regulador) | - | Alimentación (3.3V) |
| **GND** | **G** (GND) | - | Tierra |
| **CS** | **D8** | GPIO15 | Chip Select (SPI SS) |
| **RST / RESET** | **D4** | GPIO2 | Reset de pantalla |
| **DC / A0** | **D3** | GPIO0 | Data / Command |
| **SDA / MOSI** | **D7** | GPIO13 | Datos SPI (HSPI MOSI) |
| **SCK / SCL** | **D5** | GPIO14 | Reloj SPI (HSPI SCK) |
| **LED / BL** | **3V3** | - | Retroiluminación (Luz de fondo) |

---

## 🚀 Configuración y Puesta en Marcha

### 1. Configurar credenciales WiFi (`include/secrets.h`)

Copia la plantilla `secrets.h.example` a `secrets.h`:

```bash
cp include/secrets.h.example include/secrets.h
```

Abre `include/secrets.h` y edita:
1. `WIFI_SSID`: Nombre de tu red WiFi (2.4 GHz).
2. `WIFI_PASSWORD`: Contraseña de tu WiFi.
3. `HERDR_SERVER_HOST`: La IP local de tu Mac (ejecuta `./scripts/manage.sh status` para verla).
4. `HERDR_SERVER_PORT`: `8266` (por defecto).

> *Nota:* El archivo `secrets.h` está en el `.gitignore` para no exponer tus credenciales en el repositorio.

---

### 2. Compilar y Subir el Firmware

Conecta tu ESP8266 por USB **solo para el flasheo inicial**:

```bash
pio run -t upload
```

Una vez subido, ya puedes desconectar el ESP8266 de tu Mac y conectarlo a cualquier cargador USB de pared en tu escritorio.

---

### 3. Iniciar el Servidor WebSocket en tu Mac (`launchd`)

El proyecto incluye el servicio en segundo plano de macOS para arrancar el servidor WebSocket automáticamente:

```bash
./scripts/manage.sh install
```

Este comando:
1. Instala el LaunchAgent en `~/Library/LaunchAgents/com.herdr.esp8266-bridge.plist`.
2. Inicia el servidor WebSocket en el puerto `8266`.
3. Lee el socket Unix de Herdr (`~/.config/herdr/herdr.sock`) y transmite los eventos a la pantalla.

---

## 🛠️ Comandos de Gestión (`manage.sh`)

| Acción | Comando |
| :--- | :--- |
| **Ver estado y la IP local de tu Mac** | `./scripts/manage.sh status` |
| **Ver logs en tiempo real** | `./scripts/manage.sh logs` |
| **Detener el servidor** | `./scripts/manage.sh stop` |
| **Iniciar el servidor** | `./scripts/manage.sh start` |
| **Reiniciar el servidor** | `./scripts/manage.sh restart` |
| **Actualizar/Flashear firmware** | `./scripts/manage.sh flash` |

---

## 📁 Ubicación de Archivos en el Sistema

* **LaunchAgent Plist:**  
  `~/Library/LaunchAgents/com.herdr.esp8266-bridge.plist`
* **Archivo de Log estándar:**  
  `~/.config/herdr/esp8266-bridge.log`
* **Archivo de Log de errores:**  
  `~/.config/herdr/esp8266-bridge.err`
* **Socket de Herdr:**  
  `~/.config/herdr/herdr.sock`
