#!/bin/bash
# ==============================================================================
# Script de Gestión para Herdr ESP8266 WiFi WebSocket Server
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

PLIST_SRC="$PROJECT_DIR/bridge/launchd/com.herdr.esp8266-bridge.plist"
PLIST_DST="$HOME/Library/LaunchAgents/com.herdr.esp8266-bridge.plist"
LOG_OUT="$HOME/.config/herdr/esp8266-bridge.log"
LOG_ERR="$HOME/.config/herdr/esp8266-bridge.err"

case "$1" in
    install)
        echo "==> Instalando LaunchAgent en macOS..."
        mkdir -p "$HOME/Library/LaunchAgents"
        mkdir -p "$HOME/.config/herdr"

        sed -e "s|{{PROJECT_DIR}}|$PROJECT_DIR|g" \
            -e "s|{{HOME}}|$HOME|g" \
            "$PLIST_SRC" > "$PLIST_DST"

        launchctl unload "$PLIST_DST" 2>/dev/null
        launchctl load "$PLIST_DST"
        echo "✔ Servicio instalado y arrancado correctamente."
        echo "  Ruta generada: $PLIST_DST"
        ;;

    start)
        echo "==> Iniciando servicio bridge..."
        launchctl load "$PLIST_DST" 2>/dev/null
        echo "✔ Servicio iniciado."
        ;;

    stop)
        echo "==> Deteniendo servicio bridge..."
        launchctl unload "$PLIST_DST" 2>/dev/null
        echo "✔ Servicio detenido."
        ;;

    restart)
        echo "==> Reiniciando servicio bridge..."
        launchctl unload "$PLIST_DST" 2>/dev/null
        sleep 0.5
        launchctl load "$PLIST_DST" 2>/dev/null
        echo "✔ Servicio reiniciado."
        ;;

    status)
        echo "==> Estado del servicio en launchd:"
        launchctl list | grep "com.herdr.esp8266-bridge" || echo "  El servicio no está cargado."
        echo ""
        echo "==> IPs locales de tu Mac (para secrets.h):"
        ifconfig | grep "inet " | grep -v 127.0.0.1 | awk '{print "  IP: " $2}'
        echo "  Puerto WebSocket: 8266"
        ;;

    flash)
        echo "==> Flasheando firmware WiFi al ESP8266 con PlatformIO..."
        cd "$PROJECT_DIR" && pio run -t upload
        ;;

    logs)
        echo "==> Mostrando logs en tiempo real (Ctrl+C para salir)..."
        touch "$LOG_OUT" "$LOG_ERR"
        tail -f "$LOG_OUT" "$LOG_ERR"
        ;;

    *)
        echo "Uso: $0 {install|start|stop|restart|status|flash|logs}"
        echo ""
        echo "Comandos:"
        echo "  install    : Instala e inicia el LaunchAgent en macOS (~/Library/LaunchAgents)"
        echo "  start      : Inicia el servidor WebSocket de Herdr"
        echo "  stop       : Detiene el servidor"
        echo "  restart    : Reinicia el servidor"
        echo "  status     : Muestra si el servicio está corriendo y las IPs locales de tu Mac"
        echo "  flash      : Sube el firmware WiFi con PlatformIO"
        echo "  logs       : Sigue los logs en tiempo real (~/.config/herdr/esp8266-bridge.log)"
        exit 1
        ;;
esac
