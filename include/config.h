#pragma once

#include <Arduino.h>

// ==========================================
// CONFIGURACIÓN DE PANTALLA
// ==========================================
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       160
#define TFT_ROTATION        0

// ==========================================
// PALETA DE COLORES (RGB565)
// ==========================================
#define COLOR_BG            0x0842 // Fondo muy oscuro #0b0e14
#define COLOR_HEADER_BG     0x10A4 // Fondo cabecera #121820
#define COLOR_CARD_BG       0x18E5 // Fondo tarjetas #1a222c
#define COLOR_TEXT_WHITE    0xFFFF // Blanco #ffffff
#define COLOR_TEXT_MUTED    0x7BEF // Gris medio #7c8798
#define COLOR_TEXT_DARK     0x4228 // Gris oscuro para marcos

// Colores de estado
#define COLOR_WORKING       0x3E1F // Cyan neón #38bdf8 (Pensando / Procesando)
#define COLOR_TOOL          0x9BFF // Violeta / Índigo #818cf8 (Escribiendo / Tool)
#define COLOR_BLOCKED       0xFD20 // Naranja cálido #fb923c (Pregunta / Bloqueado)
#define COLOR_IDLE          0x25C9 // Verde esmeralda #22c55e (Sesión lista)
#define COLOR_OFFLINE       0x632C // Gris apagado #64748b (Standby / Desconectado)

// ==========================================
// PARÁMETROS SERIAL Y PROTOCOLO
// ==========================================
#define SERIAL_BAUD_RATE    115200
#define SERIAL_BUFFER_SIZE  512
#define HEARTBEAT_TIMEOUT   4000UL // Si en 4s no llegan datos, pasa a Standby
