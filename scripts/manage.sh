#!/bin/bash
# ==============================================================================
# Script de Gestión para Herdr ESP8266 Bridge y Puerto Serie
# ==============================================================================

PLIST_SRC="/Users/jriquelme/Desarrollo/poc/esp8266-herdr/bridge/launchd/com.herdr.esp8266-bridge.plist"
PLIST_DST="$HOME/Library/LaunchAgents/com.herdr.esp8266-bridge.plist"
LOG_OUT="$HOME/.config/herdr/esp8266-bridge.log"
LOG_ERR="$HOME/.config/herdr/esp8266-bridge.err"
PROJECT_DIR="/Users/jriquelme/Desarrollo/poc/esp8266-herdr"

case "$1" in
    install)
        echo "==> Instalando LaunchAgent en macOS..."
        mkdir -p "$HOME/Library/LaunchAgents"
        mkdir -p "$HOME/.config/herdr"
        cp "$PLIST_SRC" "$PLIST_DST"
        launchctl unload "$PLIST_DST" 2>/dev/null
        launchctl load "$PLIST_DST"
        echo "✔ Servicio instalado y arrancado correctamente."
        echo "  Ruta: $PLIST_DST"
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
        echo "==> Puertos serie USB detectados:"
        ls -l /dev/cu.usbserial* /dev/cu.wchusbserial* /dev/cu.usbmodem* 2>/dev/null || echo "  No hay puertos serie USB conectados."
        ;;

    free-port)
        echo "==> Liberando puertos serie para otros proyectos o flasheo..."
        launchctl unload "$PLIST_DST" 2>/dev/null
        
        # Buscar procesos node o serial que tengan tomado el puerto
        PIDS=$(lsof -t /dev/cu.usbserial* /dev/cu.wchusbserial* /dev/cu.usbmodem* 2>/dev/null)
        if [ -n "$PIDS" ]; then
            echo "  Cerrando procesos bloqueando el puerto: $PIDS"
            kill -9 $PIDS 2>/dev/null
        fi
        echo "✔ Puertos serie completamente libres."
        ;;

    flash)
        echo "==> Flasheando ESP8266 con PlatformIO de forma segura..."
        echo "  1. Pausando bridge..."
        launchctl unload "$PLIST_DST" 2>/dev/null
        sleep 0.5

        echo "  2. Compilando y subiendo firmware..."
        cd "$PROJECT_DIR" && pio run -t upload

        UPLOAD_STATUS=$?
        if [ $UPLOAD_STATUS -eq 0 ]; then
            echo "✔ Flasheo exitoso."
        else
            echo "✖ Hubo un error al flashear."
        fi

        echo "  3. Reanudando bridge..."
        sleep 1
        launchctl load "$PLIST_DST" 2>/dev/null
        echo "✔ Bridge reactivado."
        ;;

    logs)
        echo "==> Mostrando logs en tiempo real (Ctrl+C para salir)..."
        touch "$LOG_OUT" "$LOG_ERR"
        tail -f "$LOG_OUT" "$LOG_ERR"
        ;;

    *)
        echo "Uso: $0 {install|start|stop|restart|status|free-port|flash|logs}"
        echo ""
        echo "Comandos:"
        echo "  install    : Instala e inicia el LaunchAgent en macOS (~/Library/LaunchAgents)"
        echo "  start      : Inicia el bridge"
        echo "  stop       : Detiene el bridge y libera el USB"
        echo "  restart    : Reinicia el bridge"
        echo "  status     : Muestra si el servicio está corriendo y qué puertos USB hay"
        echo "  free-port  : Libera forzosamente cualquier puerto serie"
        echo "  flash      : Pausa el bridge, sube el firmware con PlatformIO y reanuda el bridge"
        echo "  logs       : Sigue los logs en tiempo real (~/.config/herdr/esp8266-bridge.log)"
        exit 1
        ;;
esac
