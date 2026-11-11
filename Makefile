# ==============================================================================
# Makefile - Herdr ESP8266 TFT Companion (Rama USB)
# ==============================================================================

SHELL := /bin/bash
PROJECT_DIR := $(shell pwd)
SCRIPTS_DIR := $(PROJECT_DIR)/scripts
BRIDGE_DIR := $(PROJECT_DIR)/bridge

.PHONY: all help build upload flash monitor clean deps install start stop restart status logs free-port

# Target por defecto: ayuda
all: help

help:
	@echo "======================================================================"
	@echo "       Herdr ESP8266 TFT Companion - Comandos Disponibles"
	@echo "======================================================================"
	@echo " Firmware (PlatformIO):"
	@echo "   make build       - Compila el firmware del ESP8266"
	@echo "   make flash       - Pausa el bridge, sube el firmware y reactiva el bridge"
	@echo "   make upload      - Sube directamente el firmware vía pio"
	@echo "   make monitor     - Abre el monitor serie de PlatformIO (115200)"
	@echo "   make clean       - Limpia los archivos temporales de compilación"
	@echo ""
	@echo " Bridge y Servicio en macOS (launchd):"
	@echo "   make deps        - Instala dependencias npm del bridge en bridge/"
	@echo "   make install     - Instala e inicia el LaunchAgent (~/Library/LaunchAgents)"
	@echo "   make start       - Inicia el servicio en segundo plano"
	@echo "   make stop        - Detiene el servicio y libera el puerto USB"
	@echo "   make restart     - Reinicia el servicio bridge"
	@echo "   make status      - Muestra el estado del servicio y los puertos USB conectados"
	@echo "   make logs        - Sigue los logs en tiempo real (~/.config/herdr/esp8266-bridge.log)"
	@echo "   make free-port   - Libera cualquier proceso que bloquee el puerto serie"
	@echo "======================================================================"

# ------------------------------------------------------------------------------
# Firmware ESP8266
# ------------------------------------------------------------------------------
build:
	@echo "==> Compilando firmware con PlatformIO..."
	pio run

flash:
	@echo "==> Flasheando de forma segura (sin conflictos con el bridge)..."
	@$(SCRIPTS_DIR)/manage.sh flash

upload:
	@echo "==> Subiendo firmware directamente..."
	pio run -t upload

monitor:
	@echo "==> Abriendo monitor serie (Ctrl+C para salir)..."
	pio device monitor -b 115200

clean:
	@echo "==> Limpiando archivos temporales..."
	pio run -t clean

# ------------------------------------------------------------------------------
# Bridge & macOS Service
# ------------------------------------------------------------------------------
deps:
	@echo "==> Instalando dependencias en bridge/..."
	cd $(BRIDGE_DIR) && npm install

install:
	@$(SCRIPTS_DIR)/manage.sh install

start:
	@$(SCRIPTS_DIR)/manage.sh start

stop:
	@$(SCRIPTS_DIR)/manage.sh stop

restart:
	@$(SCRIPTS_DIR)/manage.sh restart

status:
	@$(SCRIPTS_DIR)/manage.sh status

logs:
	@$(SCRIPTS_DIR)/manage.sh logs

free-port:
	@$(SCRIPTS_DIR)/manage.sh free-port
