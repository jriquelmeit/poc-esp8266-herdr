#include "serial_manager.h"
#include "config.h"
#include <ArduinoJson.h>

SerialManager::SerialManager() {
    _buffer.reserve(SERIAL_BUFFER_SIZE);
}

void SerialManager::begin() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.setRxBufferSize(SERIAL_BUFFER_SIZE);
}

void SerialManager::processLine(const String &line, HerdrData &data) {
    if (line.length() == 0) return;

    // 1. Handshake seguro contra otros dispositivos
    if (line.indexOf("HERDR_DISCOVER") >= 0) {
        Serial.println("{\"device\":\"herdr-tft\",\"v\":1,\"ready\":true}");
        return;
    }

    // 2. Parsear actualización de estado
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, line);
    if (error) {
        return;
    }

    const char* statusStr = doc["status"] | "unknown";
    if (strcmp(statusStr, "working") == 0) {
        data.status = STATUS_WORKING;
    } else if (strcmp(statusStr, "tool") == 0) {
        data.status = STATUS_TOOL;
    } else if (strcmp(statusStr, "blocked") == 0) {
        data.status = STATUS_BLOCKED;
    } else if (strcmp(statusStr, "idle") == 0) {
        data.status = STATUS_IDLE;
    } else {
        data.status = STATUS_DISCONNECTED;
    }

    data.agent = doc["agent"] | "opencode";
    data.title = doc["title"] | "Sesión activa";
    data.detail = doc["detail"] | "";

    // Parsear pestañas/tabs
    JsonArray tabsArray = doc["tabs"];
    if (!tabsArray.isNull()) {
        uint8_t count = 0;
        for (JsonObject tabObj : tabsArray) {
            if (count >= MAX_TABS) break;
            const char* s = tabObj["s"] | "i";
            if (s[0] == 'w') data.tabs[count].status = STATUS_WORKING;
            else if (s[0] == 't') data.tabs[count].status = STATUS_TOOL;
            else if (s[0] == 'b') data.tabs[count].status = STATUS_BLOCKED;
            else data.tabs[count].status = STATUS_IDLE;

            data.tabs[count].focused = tabObj["f"] | false;
            count++;
        }
        data.tabCount = count;
    }

    data.lastMessageMs = millis();
    data.isConnected = true;
}

void SerialManager::update(HerdrData &data) {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\n') {
            _buffer.trim();
            processLine(_buffer, data);
            _buffer = "";
        } else if (c != '\r') {
            if (_buffer.length() < SERIAL_BUFFER_SIZE - 1) {
                _buffer += c;
            } else {
                _buffer = ""; // Prevenir overflow
            }
        }
    }

    // Timeout de inactividad
    if (data.isConnected && (millis() - data.lastMessageMs > HEARTBEAT_TIMEOUT)) {
        data.isConnected = false;
        data.status = STATUS_DISCONNECTED;
        data.title = "Standby";
        data.detail = "Esperando Mac...";
        data.tabCount = 0;
    }
}
