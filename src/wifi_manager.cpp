#include "wifi_manager.h"
#include "config.h"
#include <ArduinoJson.h>

static WifiManager *g_wifiManagerInstance = nullptr;
static HerdrData *g_activeHerdrData = nullptr;
static bool g_wsConnected = false;

WifiManager::WifiManager()
    : _lastWifiCheckMs(0),
      _lastHeartbeatMs(0) {
    g_wifiManagerInstance = this;
}

void WifiManager::begin() {
    setupWifi();

    // Configurar WebSocket hacia la Mac
    _wsClient.begin(HERDR_SERVER_HOST, HERDR_SERVER_PORT, "/ws");
    _wsClient.onEvent(webSocketEvent);
    _wsClient.setReconnectInterval(2000);
    _wsClient.enableHeartbeat(15000, 3000, 2);
}

void WifiManager::setupWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool WifiManager::isWifiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool WifiManager::isWsConnected() const {
    return g_wsConnected;
}

void WifiManager::processJson(const uint8_t *payload, size_t length, HerdrData &data) {
    if (length == 0) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, length);
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

void WifiManager::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            g_wsConnected = false;
            if (g_activeHerdrData) {
                g_activeHerdrData->isConnected = false;
                g_activeHerdrData->status = STATUS_DISCONNECTED;
                g_activeHerdrData->title = "Sin conexion";
                g_activeHerdrData->detail = "Reconectando Mac...";
            }
            break;

        case WStype_CONNECTED:
            g_wsConnected = true;
            if (g_wifiManagerInstance) {
                // Saludo inicial al servidor
                g_wifiManagerInstance->_wsClient.sendTXT("{\"cmd\":\"HERDR_DISCOVER\"}");
            }
            if (g_activeHerdrData) {
                g_activeHerdrData->title = "Herdr Conectado";
                g_activeHerdrData->detail = "Sincronizando...";
            }
            break;

        case WStype_TEXT:
            if (g_wifiManagerInstance && g_activeHerdrData) {
                g_wifiManagerInstance->processJson(payload, length, *g_activeHerdrData);
            }
            break;

        default:
            break;
    }
}

void WifiManager::update(HerdrData &data) {
    g_activeHerdrData = &data;
    uint32_t now = millis();

    // Comprobación de WiFi periódica
    if (now - _lastWifiCheckMs > 3000) {
        _lastWifiCheckMs = now;
        if (WiFi.status() != WL_CONNECTED) {
            data.isConnected = false;
            data.status = STATUS_DISCONNECTED;
            data.title = "Buscando WiFi...";
            data.detail = WIFI_SSID;
            return;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        _wsClient.loop();
    }

    // Timeout si no llegan mensajes del WebSocket
    if (data.isConnected && (now - data.lastMessageMs > HEARTBEAT_TIMEOUT)) {
        data.isConnected = false;
        data.status = STATUS_DISCONNECTED;
        data.title = "Standby (WiFi)";
        data.detail = "Esperando Herdr...";
        data.tabCount = 0;
    }
}
