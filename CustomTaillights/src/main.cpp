#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "settings.h"
#include "wifi_server.h"
#include "inputs.h"
#include "states.h"
#include "taillight.h"
#include "animations.h"

// ── Pixel buffers (owned by main, shared with TailLight objects) ─────────────
CRGB ledsLeft [LEDS_PER_SIDE];
CRGB ledsRight[LEDS_PER_SIDE];

// ── Subsystem objects ────────────────────────────────────────────────────────
Inputs    inputs;
TailLight leftPanel (ledsLeft,  true);
TailLight rightPanel(ledsRight, false);

// ── Timing ───────────────────────────────────────────────────────────────────
static unsigned long lastFrameMs = 0;

// ── Preview state (written by wifi_server, read here) ────────────────────────
// A non-zero g_preview_until_ms means we are temporarily overriding the
// light state so the user can preview animations from the web UI.
extern volatile LightState    g_preview_state;
extern volatile unsigned long g_preview_until_ms;

// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println(F("[taillight] boot"));

    // Load persisted settings from NVS (must happen before wifiServer.begin()
    // and before FastLED.setBrightness)
    settings_load();

    // Register LED panels with FastLED
    FastLED.addLeds<LED_CHIPSET, PIN_LED_LEFT,  LED_COLOR_ORDER>(ledsLeft,  LEDS_PER_SIDE);
    FastLED.addLeds<LED_CHIPSET, PIN_LED_RIGHT, LED_COLOR_ORDER>(ledsRight, LEDS_PER_SIDE);
    FastLED.setBrightness(g_settings.brightness);
    FastLED.clear(true);

    // Initialise animation registry
    AnimationRegistry::init();

    // Initialise optocoupler inputs
    inputs.begin();

    // Initialise taillight panels (sets idle animation)
    leftPanel.begin();
    rightPanel.begin();

    // Bring up WiFi and HTTP settings server
    wifiServer.begin();

    Serial.println(F("[taillight] ready"));
}

// ---------------------------------------------------------------------------
void loop() {
    unsigned long nowMs = millis();

    // Process any pending HTTP requests
    wifiServer.handle();

    // Read and debounce stock input signals
    inputs.update();

    // Determine active light state — honour a preview override if active
    LightState state;
    if (nowMs < g_preview_until_ms) {
        state = g_preview_state;
    } else {
        state = resolveLightState(
            inputs.brake(),
            inputs.leftTurn(),
            inputs.rightTurn(),
            inputs.reverse()
        );
    }

    // Throttle animation updates to g_settings.frame_ms
    if (nowMs - lastFrameMs >= (unsigned long)g_settings.frame_ms) {
        lastFrameMs = nowMs;

        leftPanel.update(state, nowMs);
        rightPanel.update(state, nowMs);

        FastLED.show();
    }
}
