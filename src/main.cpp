#include <Arduino.h>
#include "config.h"
#include "protocol.h"
#include "serial_manager.h"
#include "display_ui.h"

static HerdrData g_herdrData;
static SerialManager g_serialManager;
static DisplayUI g_displayUI;

void setup() {
    g_serialManager.begin();
    g_displayUI.begin();
}

void loop() {
    // Procesar tramas seriales entrantes
    g_serialManager.update(g_herdrData);

    // Actualizar animaciones, textos y pantalla
    g_displayUI.update(g_herdrData);

    // Ceder tiempo al sistema operativo del ESP8266 (evita WDT)
    yield();
}
