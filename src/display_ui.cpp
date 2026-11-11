#include "display_ui.h"
#include "config.h"

DisplayUI::DisplayUI()
    : _tft(),
      _animSprite(&_tft),
      _lastStatus((HerdrStatus)99),
      _lastTitle(""),
      _lastDetail(""),
      _lastTabCount(99),
      _lastAnimMs(0),
      _animFrame(0),
      _marqueeOffset(0),
      _lastMarqueeMs(0),
      _forceRedraw(true) {}

void DisplayUI::begin() {
    _tft.init();
    _tft.setRotation(TFT_ROTATION);
    _tft.fillScreen(COLOR_BG);

    // Crear sprite en RAM de 48x48 píxeles (~4.6KB de RAM) para animaciones flicker-free
    _animSprite.createSprite(48, 48);
    _animSprite.setSwapBytes(false);

    renderStaticLayout();
}

void DisplayUI::renderStaticLayout() {
    _tft.fillScreen(COLOR_BG);

    // Barra superior
    _tft.fillRect(0, 0, SCREEN_WIDTH, 20, COLOR_HEADER_BG);
    _tft.drawFastHLine(0, 20, SCREEN_WIDTH, COLOR_CARD_BG);

    // Cabecera texto
    _tft.setTextColor(COLOR_TEXT_WHITE, COLOR_HEADER_BG);
    _tft.setTextDatum(ML_DATUM);
    _tft.drawString("HERDR", 6, 10, 2);

    // Separador inferior del footer
    _tft.drawFastHLine(0, 142, SCREEN_WIDTH, COLOR_CARD_BG);
    _tft.fillRect(0, 143, SCREEN_WIDTH, 17, COLOR_HEADER_BG);

    _forceRedraw = true;
}

void DisplayUI::update(const HerdrData &data) {
    uint32_t now = millis();

    // Redibujar fondo si cambió el estado principal
    if (data.status != _lastStatus || _forceRedraw) {
        updateHeader(data);
        _lastStatus = data.status;
        _forceRedraw = false;
        _marqueeOffset = 0;
    }

    // Actualizar animación a ~30-40 FPS (cada 25ms)
    if (now - _lastAnimMs >= 25) {
        _lastAnimMs = now;
        _animFrame++;
        updateAnimation(data);
    }

    // Actualizar info de sesión y scroll de texto suave
    if (now - _lastMarqueeMs >= 40) {
        _lastMarqueeMs = now;
        updateSessionInfo(data);
    }

    // Actualizar barra de tabs
    updateTabsFooter(data);
}

void DisplayUI::updateHeader(const HerdrData &data) {
    uint16_t statusColor = COLOR_OFFLINE;
    const char* tag = "OFFLINE";

    switch (data.status) {
        case STATUS_WORKING:
            statusColor = COLOR_WORKING;
            tag = "WORK";
            break;
        case STATUS_TOOL:
            statusColor = COLOR_TOOL;
            tag = "TOOL";
            break;
        case STATUS_BLOCKED:
            statusColor = COLOR_BLOCKED;
            tag = "WAIT";
            break;
        case STATUS_IDLE:
            statusColor = COLOR_IDLE;
            tag = "READY";
            break;
        default:
            statusColor = COLOR_OFFLINE;
            tag = "OFF";
            break;
    }

    // Badge de estado en la esquina superior derecha
    _tft.fillRoundRect(SCREEN_WIDTH - 46, 3, 40, 14, 3, statusColor);
    _tft.setTextColor(COLOR_BG, statusColor);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString(tag, SCREEN_WIDTH - 26, 10, 1);
}

void DisplayUI::updateAnimation(const HerdrData &data) {
    _animSprite.fillSprite(COLOR_BG);

    int16_t cx = 24;
    int16_t cy = 24;

    switch (data.status) {
        case STATUS_WORKING: {
            // Radar neón rotatorio con partículas
            float angle = (_animFrame * 0.12f);
            for (uint8_t r = 6; r <= 20; r += 7) {
                _animSprite.drawCircle(cx, cy, r, COLOR_CARD_BG);
            }

            // Brazo rotatorio
            int16_t x1 = cx + (int16_t)(cos(angle) * 20.0f);
            int16_t y1 = cy + (int16_t)(sin(angle) * 20.0f);
            _animSprite.drawLine(cx, cy, x1, y1, COLOR_WORKING);

            // Partícula satélite
            int16_t px = cx + (int16_t)(cos(angle + 1.2f) * 14.0f);
            int16_t py = cy + (int16_t)(sin(angle + 1.2f) * 14.0f);
            _animSprite.fillCircle(px, py, 2, COLOR_TEXT_WHITE);

            // Núcleo brillante
            uint8_t pulse = 2 + (_animFrame % 10) / 4;
            _animSprite.fillCircle(cx, cy, pulse, COLOR_WORKING);
            break;
        }

        case STATUS_TOOL: {
            // Animación de engranaje / cursor dinámico
            float a = (_animFrame * 0.08f);
            _animSprite.drawCircle(cx, cy, 18, COLOR_TOOL);
            _animSprite.drawCircle(cx, cy, 14, COLOR_CARD_BG);

            // Dientes de engranaje
            for (int i = 0; i < 6; i++) {
                float ta = a + (i * (3.14159f / 3.0f));
                int16_t tx = cx + (int16_t)(cos(ta) * 20.0f);
                int16_t ty = cy + (int16_t)(sin(ta) * 20.0f);
                _animSprite.fillCircle(tx, ty, 2, COLOR_TOOL);
            }

            // Cursor parpadeante central
            if ((_animFrame / 12) % 2 == 0) {
                _animSprite.fillRect(cx - 6, cy - 2, 12, 4, COLOR_TEXT_WHITE);
            }
            break;
        }

        case STATUS_BLOCKED: {
            // Campana / alerta pulsante de atención
            uint8_t pulse = 16 + (sin(_animFrame * 0.2f) * 4.0f);
            _animSprite.drawRoundRect(cx - pulse, cy - pulse, pulse * 2, pulse * 2, 4, COLOR_BLOCKED);
            _animSprite.fillTriangle(cx, cy - 10, cx - 10, cy + 10, cx + 10, cy + 10, COLOR_BLOCKED);
            _animSprite.fillRect(cx - 1, cy - 3, 2, 6, COLOR_BG);
            _animSprite.fillCircle(cx, cy + 6, 1, COLOR_BG);
            break;
        }

        case STATUS_IDLE: {
            // Checkmark suave con halo armónico
            uint8_t halo = 18 + (sin(_animFrame * 0.08f) * 2.0f);
            _animSprite.drawCircle(cx, cy, halo, COLOR_CARD_BG);
            _animSprite.fillCircle(cx, cy, 16, COLOR_IDLE);

            // Checkmark blanco
            _animSprite.drawLine(cx - 6, cy, cx - 1, cy + 5, COLOR_TEXT_WHITE);
            _animSprite.drawLine(cx - 1, cy + 5, cx + 7, cy - 5, COLOR_TEXT_WHITE);
            _animSprite.drawLine(cx - 6, cy - 1, cx - 1, cy + 4, COLOR_TEXT_WHITE);
            _animSprite.drawLine(cx - 1, cy + 4, cx + 7, cy - 6, COLOR_TEXT_WHITE);
            break;
        }

        case STATUS_DISCONNECTED:
        default: {
            // Avatar durmiendo (ojos en arco y Z flotando)
            _animSprite.drawCircle(cx, cy, 18, COLOR_OFFLINE);
            // Ojos durmiendo ^ ^
            _animSprite.drawLine(cx - 8, cy - 2, cx - 4, cy - 5, COLOR_TEXT_MUTED);
            _animSprite.drawLine(cx - 4, cy - 5, cx, cy - 2, COLOR_TEXT_MUTED);
            _animSprite.drawLine(cx + 2, cy - 2, cx + 6, cy - 5, COLOR_TEXT_MUTED);
            _animSprite.drawLine(cx + 6, cy - 5, cx + 10, cy - 2, COLOR_TEXT_MUTED);
            // Sonrisa sutil
            _animSprite.drawFastHLine(cx - 3, cy + 5, 7, COLOR_TEXT_MUTED);

            // Animación de Z subiendo
            int16_t zOff = (_animFrame % 40);
            int16_t zY = cy - 6 - (zOff / 2);
            int16_t zX = cx + 10 + (zOff / 3);
            if (zY > 2 && zX < 44) {
                _animSprite.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
                _animSprite.setTextDatum(MC_DATUM);
                _animSprite.drawString("z", zX, zY, 1);
            }
            break;
        }
    }

    // Volcar sprite a la pantalla centrada horizontalmente (X: 40, Y: 30)
    _animSprite.pushSprite(40, 28);

    // Texto de acción bajo el sprite (Y: 82)
    _tft.fillRect(4, 82, SCREEN_WIDTH - 8, 14, COLOR_BG);
    _tft.setTextDatum(MC_DATUM);

    switch (data.status) {
        case STATUS_WORKING:
            _tft.setTextColor(COLOR_WORKING, COLOR_BG);
            _tft.drawString("PROCESANDO...", 64, 88, 1);
            break;
        case STATUS_TOOL:
            _tft.setTextColor(COLOR_TOOL, COLOR_BG);
            _tft.drawString(data.detail.length() > 0 ? data.detail : "EJECUTANDO...", 64, 88, 1);
            break;
        case STATUS_BLOCKED:
            _tft.setTextColor(COLOR_BLOCKED, COLOR_BG);
            _tft.drawString("REQUIERE ACCION", 64, 88, 1);
            break;
        case STATUS_IDLE:
            _tft.setTextColor(COLOR_IDLE, COLOR_BG);
            _tft.drawString("SESION LISTA", 64, 88, 1);
            break;
        default:
            _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
            _tft.drawString("STANDBY", 64, 88, 1);
            break;
    }
}

void DisplayUI::updateSessionInfo(const HerdrData &data) {
    bool infoChanged = (data.title != _lastTitle);

    if (infoChanged) {
        _lastTitle = data.title;
        _marqueeOffset = 0;
        _tft.fillRect(4, 102, SCREEN_WIDTH - 8, 36, COLOR_BG);
    }

    int16_t textWidth = _tft.textWidth(data.title.c_str(), 2);
    const int16_t boxWidth = SCREEN_WIDTH - 12;

    _tft.setTextColor(COLOR_TEXT_WHITE, COLOR_BG);

    if (textWidth <= boxWidth) {
        if (infoChanged) {
            _tft.setTextDatum(MC_DATUM);
            _tft.drawString(data.title, 64, 114, 2);
        }
    } else {
        // Scroll horizontal suave (Marquee)
        _marqueeOffset -= 1;
        if (_marqueeOffset < -(textWidth + 20)) {
            _marqueeOffset = 10;
        }

        // Crear una franja limpia y dibujar con offset
        _tft.fillRect(6, 106, boxWidth, 18, COLOR_BG);
        _tft.setTextDatum(ML_DATUM);
        _tft.drawString(data.title, 6 + _marqueeOffset, 115, 2);
    }

    // Subtítulo de agente
    if (infoChanged) {
        _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
        _tft.setTextDatum(MC_DATUM);
        String sub = String("Agente: ") + data.agent;
        _tft.drawString(sub, 64, 132, 1);
    }
}

void DisplayUI::updateTabsFooter(const HerdrData &data) {
    if (data.tabCount == _lastTabCount && !_forceRedraw) {
        // Si no han cambiado las tabs, no redibujar el footer
        return;
    }
    _lastTabCount = data.tabCount;

    _tft.fillRect(0, 143, SCREEN_WIDTH, 17, COLOR_HEADER_BG);

    if (data.tabCount == 0) {
        _tft.setTextColor(COLOR_TEXT_MUTED, COLOR_HEADER_BG);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("Sin sesiones activas", 64, 151, 1);
        return;
    }

    // Calcular espaciado de píldoras
    uint8_t count = data.tabCount > MAX_TABS ? MAX_TABS : data.tabCount;
    int16_t pillW = 12;
    int16_t pillH = 6;
    int16_t spacing = 5;
    int16_t totalW = (count * pillW) + ((count - 1) * spacing);
    int16_t startX = (SCREEN_WIDTH - totalW) / 2;
    int16_t startY = 148;

    for (uint8_t i = 0; i < count; i++) {
        uint16_t color = COLOR_IDLE;
        if (data.tabs[i].status == STATUS_WORKING) color = COLOR_WORKING;
        else if (data.tabs[i].status == STATUS_TOOL) color = COLOR_TOOL;
        else if (data.tabs[i].status == STATUS_BLOCKED) color = COLOR_BLOCKED;

        int16_t x = startX + (i * (pillW + spacing));

        if (data.tabs[i].focused) {
            // Píldora activa con borde blanco y mayor relieve
            _tft.fillRoundRect(x - 1, startY - 2, pillW + 2, pillH + 4, 3, COLOR_TEXT_WHITE);
            _tft.fillRoundRect(x, startY - 1, pillW, pillH + 2, 2, color);
        } else {
            // Píldora normal
            _tft.fillRoundRect(x, startY, pillW, pillH, 2, color);
        }
    }
}
