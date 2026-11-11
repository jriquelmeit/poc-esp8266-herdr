#include <Arduino.h>
#include "config.h"
#include "protocol.h"
#include "wifi_manager.h"
#include "display_ui.h"

static HerdrData g_herdrData;
static WifiManager g_wifiManager;
static DisplayUI g_displayUI;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    g_displayUI.begin();

    g_herdrData.status = STATUS_DISCONNECTED;
    g_herdrData.title = "Iniciando WiFi...";
    g_herdrData.detail = WIFI_SSID;

    g_wifiManager.begin();
}

void loop() {
    // Procesar comunicaciones WebSocket / WiFi
    g_wifiManager.update(g_herdrData);

    // Actualizar animaciones y pantalla TFT
    g_displayUI.update(g_herdrData);

    // Ceder CPU al background stack WiFi
    yield();
}
