// ---------------------------------------------------------------------------
// animations.cpp
// Concrete animation implementations.
// ---------------------------------------------------------------------------

#include "animations.h"
#include "taillight.h"
#include "config.h"
#include "settings.h"

// ── Static instance definitions ─────────────────────────────────────────────
AnimOff        AnimationRegistry::_off;
AnimRunning    AnimationRegistry::_running;
AnimBrake      AnimationRegistry::_brake;
AnimTurnSignal AnimationRegistry::_turnLeft;
AnimTurnSignal AnimationRegistry::_turnRight;
AnimReverse    AnimationRegistry::_reverse;
AnimHazard     AnimationRegistry::_hazard;

// ---------------------------------------------------------------------------
void AnimationRegistry::init() {
    // Nothing to allocate dynamically for the built-ins.
    // Extend here when adding animations that need heap allocation.
}

Animation* AnimationRegistry::get(LightState state, bool isLeft) {
    switch (state) {
        case LightState::BRAKE:       return &_brake;
        case LightState::LEFT_TURN:   return isLeft ? &_turnLeft : &_off;
        case LightState::RIGHT_TURN:  return isLeft ? &_off      : &_turnRight;
        case LightState::REVERSE:     return &_reverse;
        case LightState::BRAKE_LEFT:  return isLeft ? &_turnLeft : &_brake;
        case LightState::BRAKE_RIGHT: return isLeft ? &_brake    : &_turnRight;
        case LightState::HAZARD:      return &_hazard;
        case LightState::OFF:
        default:                      return &_off;
    }
}

// ── AnimOff ──────────────────────────────────────────────────────────────────
void AnimOff::update(TailLight& side, LightState /*state*/, unsigned long /*nowMs*/) {
    side.fill(CRGB::Black);
}

// ── AnimRunning ──────────────────────────────────────────────────────────────
void AnimRunning::update(TailLight& side, LightState /*state*/, unsigned long /*nowMs*/) {
    // Dim red parking-light glow — level comes from runtime settings
    side.fill(CRGB(g_settings.brightness_dim, 0, 0));
}

// ── AnimBrake ────────────────────────────────────────────────────────────────
void AnimBrake::update(TailLight& side, LightState /*state*/, unsigned long /*nowMs*/) {
    side.fill(CRGB(g_settings.brake_r, g_settings.brake_g, g_settings.brake_b));
}

// ── AnimTurnSignal ───────────────────────────────────────────────────────────
// Sequential column sweep across the 8×32 matrix.
// Each cycle sweeps all 32 columns left-to-right (left side) or
// right-to-left (right side), then blanks, then repeats.

void AnimTurnSignal::begin(TailLight& side, LightState /*state*/) {
    _startMs = millis();
    _step    = 0;
}

void AnimTurnSignal::update(TailLight& side, LightState /*state*/, unsigned long nowMs) {
    unsigned long elapsed    = nowMs - _startMs;
    unsigned long halfPeriod = g_settings.turn_blink_ms / 2;

    // Which half of the blink cycle are we in?
    unsigned long phase = elapsed % g_settings.turn_blink_ms;

    if (phase >= halfPeriod) {
        // Blank / off half
        side.fill(CRGB::Black);
        _step = 0;
        return;
    }

    // Sweep half: illuminate columns 0..step progressively
    _step = static_cast<int>((phase * MATRIX_COLS) / halfPeriod);
    _step = constrain(_step, 0, MATRIX_COLS - 1);

    side.fill(CRGB::Black);

    bool sweepForward = side.isLeft();  // left panel sweeps inward (left→right)

    for (int col = 0; col <= _step; col++) {
        int c = sweepForward ? col : (MATRIX_COLS - 1 - col);
        for (int row = 0; row < MATRIX_ROWS; row++) {
            side.setPixel(row, c, CRGB(g_settings.turn_r, g_settings.turn_g, g_settings.turn_b));
        }
    }
}

void AnimTurnSignal::end(TailLight& side) {
    side.fill(CRGB::Black);
}

// ── AnimReverse ──────────────────────────────────────────────────────────────
void AnimReverse::update(TailLight& side, LightState /*state*/, unsigned long /*nowMs*/) {
    side.fill(CRGB(g_settings.reverse_r, g_settings.reverse_g, g_settings.reverse_b));
}

// ── AnimHazard ───────────────────────────────────────────────────────────────
void AnimHazard::begin(TailLight& side, LightState /*state*/) {
    _startMs = millis();
}

void AnimHazard::update(TailLight& side, LightState /*state*/, unsigned long nowMs) {
    unsigned long phase = (nowMs - _startMs) % g_settings.turn_blink_ms;
    bool on = phase < (g_settings.turn_blink_ms / 2);
    side.fill(on ? CRGB(g_settings.turn_r, g_settings.turn_g, g_settings.turn_b) : CRGB::Black);
}
