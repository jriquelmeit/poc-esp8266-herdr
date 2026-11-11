#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include "protocol.h"

class WifiManager {
public:
    WifiManager();
    void begin();
    void update(HerdrData &data);
    bool isWifiConnected() const;
    bool isWsConnected() const;

private:
    void setupWifi();
    void processJson(const uint8_t *payload, size_t length, HerdrData &data);
    static void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

    WebSocketsClient _wsClient;
    uint32_t _lastWifiCheckMs;
    uint32_t _lastHeartbeatMs;
};
