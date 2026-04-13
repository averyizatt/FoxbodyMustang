#pragma once

// ---------------------------------------------------------------------------
// taillight.h
// Represents one physical 8×32 WS2812B LED panel.
// Owns the pixel buffer and exposes helpers used by animations.
// ---------------------------------------------------------------------------

#include <FastLED.h>
#include "config.h"
#include "states.h"
#include "animations.h"

class TailLight {
public:
    // `pixels`  — pointer to the CRGB array owned by main.cpp
    // `isLeft`  — true for the driver-side (left) panel
    TailLight(CRGB* pixels, bool isLeft)
        : _pixels(pixels), _isLeft(isLeft) {}

    // Call once in setup() after FastLED.addLeds() has been called
    void begin();

    // Call every loop iteration with the current system state and timestamp.
    void update(LightState state, unsigned long nowMs);

    // ── Helpers used by Animation subclasses ────────────────────────────────
    bool isLeft() const { return _isLeft; }

    // Fill entire panel with one colour
    void fill(CRGB colour);

    // Set a single pixel by (row, col) coordinates.
    // Row 0 is the top row; col 0 is the left column.
    // Handles serpentine wiring automatically if SERPENTINE is defined.
    void setPixel(int row, int col, CRGB colour);

    // Raw index accessor for animations that walk the array directly
    CRGB& operator[](int index) { return _pixels[index]; }

    int numPixels() const { return LEDS_PER_SIDE; }

private:
    CRGB*      _pixels;
    bool       _isLeft;

    LightState  _currentState  = LightState::OFF;
    Animation*  _currentAnim   = nullptr;

    // Translate (row, col) to linear LED index.
    // Assumes serpentine (boustrophedon) layout: even rows left→right,
    // odd rows right→left.  Adjust if your panels differ.
    int _index(int row, int col) const;
};
