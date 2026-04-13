// ---------------------------------------------------------------------------
// main.cpp
// ESP32-S3 Custom Foxbody Mustang Taillight Controller
//
// Hardware overview
// ─────────────────
//  • Two 8×32 WS2812B LED panels (256 pixels each)
//      – Left  panel  → GPIO PIN_LED_LEFT
//      – Right panel  → GPIO PIN_LED_RIGHT
//  • 4-channel optocoupler (stock 12 V → 3.3 V isolation)
//      – CH1 Brake       → GPIO PIN_OPT_BRAKE
//      – CH2 Left turn   → GPIO PIN_OPT_LEFT_TURN
//      – CH3 Right turn  → GPIO PIN_OPT_RIGHT_TURN
//      – CH4 Reverse     → GPIO PIN_OPT_REVERSE
//
// Adding new animations
// ──────────────────────
//  1. Subclass Animation in animations.h / animations.cpp.
//  2. Register it in AnimationRegistry::get() for the desired LightState(s).
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
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

// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println(F("[taillight] boot"));

    // Register LED panels with FastLED
    FastLED.addLeds<LED_CHIPSET, PIN_LED_LEFT,  LED_COLOR_ORDER>(ledsLeft,  LEDS_PER_SIDE);
    FastLED.addLeds<LED_CHIPSET, PIN_LED_RIGHT, LED_COLOR_ORDER>(ledsRight, LEDS_PER_SIDE);
    FastLED.setBrightness(BRIGHTNESS_DEFAULT);
    FastLED.clear(true);

    // Initialise animation registry
    AnimationRegistry::init();

    // Initialise optocoupler inputs
    inputs.begin();

    // Initialise taillight panels (sets idle animation)
    leftPanel.begin();
    rightPanel.begin();

    Serial.println(F("[taillight] ready"));
}

// ---------------------------------------------------------------------------
void loop() {
    unsigned long nowMs = millis();

    // Read and debounce stock input signals
    inputs.update();

    // Derive the active LightState from the four input flags
    LightState state = resolveLightState(
        inputs.brake(),
        inputs.leftTurn(),
        inputs.rightTurn(),
        inputs.reverse()
    );

    // Throttle animation updates to FRAME_INTERVAL_MS
    if (nowMs - lastFrameMs >= FRAME_INTERVAL_MS) {
        lastFrameMs = nowMs;

        leftPanel.update(state, nowMs);
        rightPanel.update(state, nowMs);

        FastLED.show();
    }
}
