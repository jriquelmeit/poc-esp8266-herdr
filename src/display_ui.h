#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "protocol.h"

class DisplayUI {
public:
    DisplayUI();
    void begin();
    void update(const HerdrData &data);

private:
    void renderStaticLayout();
    void updateHeader(const HerdrData &data);
    void updateAnimation(const HerdrData &data);
    void updateSessionInfo(const HerdrData &data);
    void updateTabsFooter(const HerdrData &data);

    TFT_eSPI _tft;
    TFT_eSprite _animSprite; // Sprite de 48x48 para animaciones a 60 FPS sin flicker

    HerdrStatus _lastStatus;
    String _lastTitle;
    String _lastDetail;
    uint8_t _lastTabCount;

    uint32_t _lastAnimMs;
    uint16_t _animFrame;
    int16_t _marqueeOffset;
    uint32_t _lastMarqueeMs;
    bool _forceRedraw;
};
