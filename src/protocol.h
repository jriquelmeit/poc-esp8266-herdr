#pragma once

#include <Arduino.h>

enum HerdrStatus {
    STATUS_DISCONNECTED = 0,
    STATUS_IDLE,
    STATUS_WORKING,
    STATUS_TOOL,
    STATUS_BLOCKED
};

struct TabIndicator {
    HerdrStatus status;
    bool focused;
};

#define MAX_TABS 6

struct HerdrData {
    HerdrStatus status = STATUS_DISCONNECTED;
    String title = "Desconectado";
    String agent = "herdr";
    String detail = "Esperando Mac...";
    TabIndicator tabs[MAX_TABS];
    uint8_t tabCount = 0;
    uint32_t lastMessageMs = 0;
    bool isConnected = false;
};
