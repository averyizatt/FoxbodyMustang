#pragma once

// ---------------------------------------------------------------------------
// taillight.h
// Represents one physical taillight assembly with three independent
// WS2812B LED strip sections:
//   • RUNNING — dim red parking/running light
//   • BRAKE   — bright red brake light + turn-signal (animation-driven)
//   • REVERSE — white reverse light
// ---------------------------------------------------------------------------

#include <FastLED.h>
#include "config.h"
#include "states.h"
#include "animations.h"

class TailLight {
public:
    // Each section owns its own CRGB array (allocated in main.cpp).
    // isLeft: true for the driver-side (left) assembly.
    TailLight(CRGB* runPixels, CRGB* brakePixels, CRGB* revPixels, bool isLeft)
        : _runPixels(runPixels), _brakePixels(brakePixels),
          _revPixels(revPixels), _isLeft(isLeft) {}

    // Call once in setup() after all FastLED.addLeds() calls.
    void begin();

    // Call every loop iteration with the current system state and timestamp.
    void update(LightState state, unsigned long nowMs);

    // ── Helpers used by Animation subclasses (operate on the BRAKE section) ─
    bool isLeft() const { return _isLeft; }

    // Fill the entire brake section with one colour
    void fill(CRGB colour);

    // Set a single pixel in the brake section by linear index
    void setPixel(int index, CRGB colour);

    // Raw accessor and size for the brake section
    CRGB& operator[](int index) { return _brakePixels[index]; }
    int   numPixels() const { return LEDS_PER_SECTION_BRAKE; }

private:
    CRGB* _runPixels;
    CRGB* _brakePixels;
    CRGB* _revPixels;
    bool  _isLeft;

    LightState _currentState = LightState::OFF;
    Animation* _brakeAnim    = nullptr;

    // Fill a section buffer with one colour
    static void _fillSection(CRGB* buf, int len, CRGB colour);
};
