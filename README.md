# Herdr ESP8266 TFT Companion (1.77" ST7735)

Monitor de escritorio físico para **Herdr** y agentes de IA de codificación (como **OpenCode**). Muestra en tiempo real lo que está haciendo la IA (pensando, ejecutando herramientas, esperando confirmación, sesión lista) con animaciones fluidas a 30-60 FPS en una pantalla TFT ST7735 de 1.77" (128x160) sobre un NodeMCU ESP8266.

---

## 📋 Características

* **Animaciones Fluidas:** Uso de `TFT_eSPI` con sprites de renderizado en memoria RAM para 0% parpadeo (*flicker-free*).
* **Estados en Tiempo Real:**
  * 🌀 **Procesando / Pensando (`working`):** Radar orbital neón con satélite rotatorio y partículas.
  * ⚙️ **Escribiendo / Herramienta (`tool`):** Engranaje animado con cursor parpadeante y nombre de la herramienta/detalle.
  * 🔔 **Requiere Atención / Pregunta (`blocked`):** Campana/alerta pulsante en ámbar.
  * ✅ **Sesión Lista (`idle`):** Círculo verde con halo armónico y checkmark.
  * 💤 **Standby / Desconectado:** Avatar durmiendo si la Mac entra en reposo o el cable se desconecta.
* **Barra inferior multi-sesión:** Indicadores tipo píldora (dots) con el estado de cada tab/pane activo en Herdr.
* **Marquee Suave:** Títulos largos de sesión se desplazan suavemente sin cortar el texto.
* **Handshake Seguro Multi-Dispositivo:** El puente en la Mac **nunca interfiere con tus otros proyectos ESP32/ESP8266**. Antes de usar un puerto, envía `HERDR_DISCOVER`. Si el dispositivo no responde con la firma de Herdr en milisegundos, el puerto se libera y se ignora de inmediato.

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

> *Nota:* La velocidad del bus SPI está fijada en **27 MHz** y la CPU del ESP8266 en **160 MHz** en `platformio.ini` para máximo rendimiento visual.

---

## 🚀 Puesta en Marcha Rápida

### 1. Compilar y Subir el Firmware al ESP8266

Conecta tu ESP8266 por USB y ejecuta desde la raíz de este proyecto:

```bash
pio run -t upload
```

O si prefieres monitorear la salida serie:

```bash
pio run -t upload -t monitor
```

---

### 2. Instalar el Servicio Automático en macOS (`launchd`)

El proyecto incluye un script de administración que configura el servicio en segundo plano de macOS para que arranque solo con tu sesión:

```bash
./scripts/manage.sh install
```

Este comando:
1. Copia el archivo `.plist` a `~/Library/LaunchAgents/com.herdr.esp8266-bridge.plist`.
2. Lo registra en el sistema con `launchctl load`.
3. Empieza a monitorear en segundo plano silenciosamente.

---

## 🛠️ Comandos de Gestión y Control

Puedes usar el script `./scripts/manage.sh` o los comandos nativos de `launchctl`:

### Usando el script `manage.sh` (Recomendado):

| Acción | Comando |
| :--- | :--- |
| **Instalar servicio launchd en macOS** | `make install` |
| **Desinstalar y eliminar servicio launchd** | `make uninstall` |
| **Ver estado del servicio y puertos USB** | `make status` (o `./scripts/manage.sh status`) |
| **Ver logs en tiempo real** | `./scripts/manage.sh logs` |
| **Detener el bridge** | `./scripts/manage.sh stop` |
| **Iniciar el bridge** | `./scripts/manage.sh start` |
| **Reiniciar el bridge** | `./scripts/manage.sh restart` |
| **Liberar puertos para otros proyectos** | `./scripts/manage.sh free-port` |
| **Flashear firmware sin conflictos** | `./scripts/manage.sh flash` |

> 💡 **Tip:** El comando `./scripts/manage.sh flash` pausa automáticamente el bridge de fondo, sube el firmware con PlatformIO y vuelve a iniciar el bridge sin que tengas que hacer nada manualmente.

---

### Comandos nativos de macOS (`launchctl`):

Si prefieres ejecutar los comandos de macOS directamente:

* **Apagar / Descargar el servicio:**
  ```bash
  launchctl unload ~/Library/LaunchAgents/com.herdr.esp8266-bridge.plist
  ```

* **Encender / Cargar el servicio:**
  ```bash
  launchctl load ~/Library/LaunchAgents/com.herdr.esp8266-bridge.plist
  ```

* **Comprobar si está corriendo:**
  ```bash
  launchctl list | grep "com.herdr.esp8266-bridge"
  ```

* **Ver si algún proceso tiene tomado el puerto USB:**
  ```bash
  lsof | grep /dev/cu.usbserial
  ```

* **Liberar el puerto manualmente si estuviese ocupado:**
  ```bash
  kill -9 $(lsof -t /dev/cu.usbserial* 2>/dev/null)
  ```

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

---

## 🛡️ Seguridad con Múltiples ESPs y Otros Desarrollos

Si sueles conectar otros microcontroladores (por ejemplo tu ESP32 de Spotify u otros proyectos):
1. El bridge **no secuestra puertos desconocidos**.
2. Al detectar un nuevo puerto USB, envía un sondeo discreto: `{"cmd":"HERDR_DISCOVER"}`.
3. Si el dispositivo no responde con `{"device":"herdr-tft"}` en menos de 1 segundo, **cierra el puerto inmediatamente** y lo marca como ignorado para que puedas trabajar con PlatformIO o el Arduino IDE en tu otro proyecto con total normalidad.
