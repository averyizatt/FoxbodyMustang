// ---------------------------------------------------------------------------
// settings.cpp
// Load / save / reset TaillightSettings using the ESP32 Preferences library
// (NVS — non-volatile storage).  Keys are ≤ 15 characters as required by NVS.
// ---------------------------------------------------------------------------

#include "settings.h"
#include <Preferences.h>
#include <string.h>

TaillightSettings g_settings;

static Preferences _prefs;
static constexpr char NVS_NS[] = "taillights";

// ---------------------------------------------------------------------------
void settings_load() {
    _prefs.begin(NVS_NS, /*readOnly=*/true);

    g_settings.brightness     = _prefs.getUChar ("brightness",      128);
    g_settings.brightness_dim = _prefs.getUChar ("brightness_dim",   40);
    g_settings.turn_blink_ms  = _prefs.getUShort("turn_blink_ms",   600);
    g_settings.frame_ms       = _prefs.getUChar ("frame_ms",         20);

    g_settings.brake_r        = _prefs.getUChar ("brake_r",         255);
    g_settings.brake_g        = _prefs.getUChar ("brake_g",           0);
    g_settings.brake_b        = _prefs.getUChar ("brake_b",           0);

    g_settings.turn_r         = _prefs.getUChar ("turn_r",          255);
    g_settings.turn_g         = _prefs.getUChar ("turn_g",          100);
    g_settings.turn_b         = _prefs.getUChar ("turn_b",            0);

    g_settings.reverse_r      = _prefs.getUChar ("reverse_r",       255);
    g_settings.reverse_g      = _prefs.getUChar ("reverse_g",       255);
    g_settings.reverse_b      = _prefs.getUChar ("reverse_b",       255);

    g_settings.brake_anim     = _prefs.getUChar ("brake_anim",         0);
    g_settings.turn_anim      = _prefs.getUChar ("turn_anim",          0);
    g_settings.reverse_anim   = _prefs.getUChar ("reverse_anim",       0);
    g_settings.run_anim       = _prefs.getUChar ("run_anim",           0);
    g_settings.lens_preset    = _prefs.getUChar ("lens_preset",        0);

    g_settings.wifi_mode      = _prefs.getUChar ("wifi_mode",          0);

    _prefs.getString("ap_ssid",  g_settings.ap_ssid,  sizeof(g_settings.ap_ssid));
    _prefs.getString("ap_pass",  g_settings.ap_pass,  sizeof(g_settings.ap_pass));
    _prefs.getString("sta_ssid", g_settings.sta_ssid, sizeof(g_settings.sta_ssid));
    _prefs.getString("sta_pass", g_settings.sta_pass, sizeof(g_settings.sta_pass));

    _prefs.end();

    // Apply default SSID/pass if NVS returned empty strings.
    // strncpy + explicit null-terminator guards against future buffer-size changes.
    if (g_settings.ap_ssid[0] == '\0') {
        strncpy(g_settings.ap_ssid, "Foxbody-Taillights", sizeof(g_settings.ap_ssid) - 1);
        g_settings.ap_ssid[sizeof(g_settings.ap_ssid) - 1] = '\0';
    }
    if (g_settings.ap_pass[0] == '\0') {
        strncpy(g_settings.ap_pass, "mustang87", sizeof(g_settings.ap_pass) - 1);
        g_settings.ap_pass[sizeof(g_settings.ap_pass) - 1] = '\0';
    }
}

// ---------------------------------------------------------------------------
void settings_save() {
    _prefs.begin(NVS_NS, /*readOnly=*/false);

    _prefs.putUChar ("brightness",      g_settings.brightness);
    _prefs.putUChar ("brightness_dim",  g_settings.brightness_dim);
    _prefs.putUShort("turn_blink_ms",   g_settings.turn_blink_ms);
    _prefs.putUChar ("frame_ms",        g_settings.frame_ms);

    _prefs.putUChar ("brake_r",         g_settings.brake_r);
    _prefs.putUChar ("brake_g",         g_settings.brake_g);
    _prefs.putUChar ("brake_b",         g_settings.brake_b);

    _prefs.putUChar ("turn_r",          g_settings.turn_r);
    _prefs.putUChar ("turn_g",          g_settings.turn_g);
    _prefs.putUChar ("turn_b",          g_settings.turn_b);

    _prefs.putUChar ("reverse_r",       g_settings.reverse_r);
    _prefs.putUChar ("reverse_g",       g_settings.reverse_g);
    _prefs.putUChar ("reverse_b",       g_settings.reverse_b);

    _prefs.putUChar ("brake_anim",      g_settings.brake_anim);
    _prefs.putUChar ("turn_anim",       g_settings.turn_anim);
    _prefs.putUChar ("reverse_anim",    g_settings.reverse_anim);
    _prefs.putUChar ("run_anim",        g_settings.run_anim);
    _prefs.putUChar ("lens_preset",     g_settings.lens_preset);

    _prefs.putUChar ("wifi_mode",       g_settings.wifi_mode);

    _prefs.putString("ap_ssid",         g_settings.ap_ssid);
    _prefs.putString("ap_pass",         g_settings.ap_pass);
    _prefs.putString("sta_ssid",        g_settings.sta_ssid);
    _prefs.putString("sta_pass",        g_settings.sta_pass);

    _prefs.end();
}

// ---------------------------------------------------------------------------
void settings_reset() {
    g_settings = TaillightSettings{};   // construct with compiled-in defaults
    settings_save();
}
