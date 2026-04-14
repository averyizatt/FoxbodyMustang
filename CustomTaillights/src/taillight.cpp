// ---------------------------------------------------------------------------
// taillight.cpp
// ---------------------------------------------------------------------------

#include "taillight.h"

void TailLight::begin() {
    _fillSection(_runPixels,   LEDS_PER_SECTION_RUNNING, CRGB::Black);
    _fillSection(_brakePixels, LEDS_PER_SECTION_BRAKE,   CRGB::Black);
    _fillSection(_revPixels,   LEDS_PER_SECTION_REVERSE, CRGB::Black);

    _currentState = LightState::OFF;
    _brakeAnim    = AnimationRegistry::get(LightState::OFF, _isLeft);
    if (_brakeAnim) _brakeAnim->begin(*this, _currentState);
}

void TailLight::update(LightState state, unsigned long nowMs) {
    // ── Running section: dim red whenever the system is active ───────────────
    _fillSection(_runPixels, LEDS_PER_SECTION_RUNNING,
                 (state != LightState::OFF)
                     ? CRGB(BRIGHTNESS_DIM, 0, 0)
                     : CRGB::Black);

    // ── Reverse section: white only during REVERSE ───────────────────────────
    _fillSection(_revPixels, LEDS_PER_SECTION_REVERSE,
                 (state == LightState::REVERSE)
                     ? CRGB::White
                     : CRGB::Black);

    // ── Brake section: animation-driven (brake, turn, hazard) ────────────────
    if (state != _currentState) {
        if (_brakeAnim) _brakeAnim->end(*this);

        _currentState = state;
        _brakeAnim    = AnimationRegistry::get(state, _isLeft);

        if (_brakeAnim) _brakeAnim->begin(*this, _currentState);
    }

    if (_brakeAnim) {
        _brakeAnim->update(*this, _currentState, nowMs);
    }
}

void TailLight::fill(CRGB colour) {
    _fillSection(_brakePixels, LEDS_PER_SECTION_BRAKE, colour);
}

void TailLight::setPixel(int index, CRGB colour) {
    if (index < 0 || index >= LEDS_PER_SECTION_BRAKE) return;
    _brakePixels[index] = colour;
}

void TailLight::_fillSection(CRGB* buf, int len, CRGB colour) {
    for (int i = 0; i < len; i++) {
        buf[i] = colour;
    }
}
