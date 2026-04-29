#pragma once

// ---------------------------------------------------------------------------
// settings.h
// Runtime-configurable settings stored in ESP32 NVS (via Preferences).
// All code that previously used compile-time constants from config.h for
// colours and timing should reference g_settings instead so that the web UI
// can change values without a reflash.
// ---------------------------------------------------------------------------

#include <Arduino.h>

struct TaillightSettings {
    // ── Brightness ──────────────────────────────────────────────────────────
    uint8_t  brightness     = 128;   // main LED brightness (10–255)
    uint8_t  brightness_dim =  40;   // running-light dim level (5–100)

    // ── Animation timing ────────────────────────────────────────────────────
    uint16_t turn_blink_ms  = 600;   // full on+off blink period in ms (200–1500)
    uint8_t  frame_ms       =  20;   // frame interval ≈ 50 fps (10–100)

    // ── Brake color (default: full red) ────────────────────────────────────
    uint8_t  brake_r = 255, brake_g =   0, brake_b =   0;

    // ── Turn-signal / hazard color (default: amber) ─────────────────────────
    uint8_t  turn_r  = 255, turn_g  = 100, turn_b  =   0;

    // ── Reverse color (default: white) ──────────────────────────────────────
    uint8_t  reverse_r = 255, reverse_g = 255, reverse_b = 255;

    // ── WiFi ────────────────────────────────────────────────────────────────
    uint8_t  wifi_mode = 0;                      // 0 = AP,  1 = Station
    char     ap_ssid[33]  = "Foxbody-Taillights";
    char     ap_pass[65]  = "mustang87";          // min 8 chars
    char     sta_ssid[33] = "";
    char     sta_pass[65] = "";
};

// Global settings instance — read/written by the web server, consumed by
// animations and the main loop.
extern TaillightSettings g_settings;

// Load settings from NVS (call once in setup before wifiServer.begin()).
void settings_load();

// Persist the current g_settings to NVS.
void settings_save();

// Reset g_settings to compiled-in defaults and persist.
void settings_reset();
