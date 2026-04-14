// ---------------------------------------------------------------------------
// main.cpp
// ESP32-S3 Custom Foxbody Mustang Taillight Controller
//
// Hardware overview
// ─────────────────
//  • Six WS2812B LED strip sections (three per taillight side, 60 LED/m)
//      Left  running  → GPIO PIN_LED_LEFT_RUNNING
//      Left  brake    → GPIO PIN_LED_LEFT_BRAKE
//      Left  reverse  → GPIO PIN_LED_LEFT_REVERSE
//      Right running  → GPIO PIN_LED_RIGHT_RUNNING
//      Right brake    → GPIO PIN_LED_RIGHT_BRAKE
//      Right reverse  → GPIO PIN_LED_RIGHT_REVERSE
//  • 4-channel optocoupler (stock 12 V → 3.3 V isolation)
//      CH1 Brake       → GPIO PIN_OPT_BRAKE
//      CH2 Left turn   → GPIO PIN_OPT_LEFT_TURN
//      CH3 Right turn  → GPIO PIN_OPT_RIGHT_TURN
//      CH4 Reverse     → GPIO PIN_OPT_REVERSE
//
// Section behavior
// ─────────────────
//  Running section  — dim red whenever state != OFF
//  Brake section    — animation-driven (bright red, amber sweep, amber flash)
//  Reverse section  — white during REVERSE, off otherwise
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <FastLED.h>

#include "config.h"
#include "inputs.h"
#include "states.h"
#include "taillight.h"
#include "animations.h"

// ── Pixel buffers (owned by main, shared with TailLight objects) ─────────────
CRGB ledsLeftRunning [LEDS_PER_SECTION_RUNNING];
CRGB ledsLeftBrake   [LEDS_PER_SECTION_BRAKE];
CRGB ledsLeftReverse [LEDS_PER_SECTION_REVERSE];
CRGB ledsRightRunning[LEDS_PER_SECTION_RUNNING];
CRGB ledsRightBrake  [LEDS_PER_SECTION_BRAKE];
CRGB ledsRightReverse[LEDS_PER_SECTION_REVERSE];

// ── Subsystem objects ────────────────────────────────────────────────────────
Inputs    inputs;
TailLight leftLight (ledsLeftRunning,  ledsLeftBrake,  ledsLeftReverse,  true);
TailLight rightLight(ledsRightRunning, ledsRightBrake, ledsRightReverse, false);

// ── Timing ───────────────────────────────────────────────────────────────────
static unsigned long lastFrameMs = 0;

// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println(F("[taillight] boot"));

    // Register all six LED strip sections with FastLED
    FastLED.addLeds<LED_CHIPSET, PIN_LED_LEFT_RUNNING,  LED_COLOR_ORDER>(ledsLeftRunning,  LEDS_PER_SECTION_RUNNING);
    FastLED.addLeds<LED_CHIPSET, PIN_LED_LEFT_BRAKE,    LED_COLOR_ORDER>(ledsLeftBrake,    LEDS_PER_SECTION_BRAKE);
    FastLED.addLeds<LED_CHIPSET, PIN_LED_LEFT_REVERSE,  LED_COLOR_ORDER>(ledsLeftReverse,  LEDS_PER_SECTION_REVERSE);
    FastLED.addLeds<LED_CHIPSET, PIN_LED_RIGHT_RUNNING, LED_COLOR_ORDER>(ledsRightRunning, LEDS_PER_SECTION_RUNNING);
    FastLED.addLeds<LED_CHIPSET, PIN_LED_RIGHT_BRAKE,   LED_COLOR_ORDER>(ledsRightBrake,   LEDS_PER_SECTION_BRAKE);
    FastLED.addLeds<LED_CHIPSET, PIN_LED_RIGHT_REVERSE, LED_COLOR_ORDER>(ledsRightReverse, LEDS_PER_SECTION_REVERSE);

    FastLED.setBrightness(BRIGHTNESS_DEFAULT);
    FastLED.clear(true);

    // Initialise animation registry
    AnimationRegistry::init();

    // Initialise optocoupler inputs
    inputs.begin();

    // Initialise taillights (sets idle state on all sections)
    leftLight.begin();
    rightLight.begin();

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

        leftLight.update(state, nowMs);
        rightLight.update(state, nowMs);

        FastLED.show();
    }
}
