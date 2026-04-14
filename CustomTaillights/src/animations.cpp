// ---------------------------------------------------------------------------
// animations.cpp
// Concrete animation implementations for the BRAKE strip section.
// ---------------------------------------------------------------------------

#include "animations.h"
#include "taillight.h"
#include "config.h"

// ── Static instance definitions ─────────────────────────────────────────────
AnimOff        AnimationRegistry::_off;
AnimBrake      AnimationRegistry::_brake;
AnimTurnSignal AnimationRegistry::_turnLeft;
AnimTurnSignal AnimationRegistry::_turnRight;
AnimHazard     AnimationRegistry::_hazard;

// ---------------------------------------------------------------------------
void AnimationRegistry::init() {
    // Nothing to allocate dynamically for the built-ins.
    // Extend here when adding animations that need heap allocation.
}

Animation* AnimationRegistry::get(LightState state, bool isLeft) {
    switch (state) {
        case LightState::BRAKE:       return &_brake;
        case LightState::LEFT_TURN:   return isLeft ? &_turnLeft  : &_off;
        case LightState::RIGHT_TURN:  return isLeft ? &_off       : &_turnRight;
        case LightState::BRAKE_LEFT:  return isLeft ? &_turnLeft  : &_brake;
        case LightState::BRAKE_RIGHT: return isLeft ? &_brake     : &_turnRight;
        case LightState::HAZARD:      return &_hazard;
        case LightState::REVERSE:
        case LightState::OFF:
        default:                      return &_off;
    }
}

// ── AnimOff ──────────────────────────────────────────────────────────────────
void AnimOff::update(TailLight& side, LightState /*state*/, unsigned long /*nowMs*/) {
    side.fill(CRGB::Black);
}

// ── AnimBrake ────────────────────────────────────────────────────────────────
void AnimBrake::update(TailLight& side, LightState /*state*/, unsigned long /*nowMs*/) {
    side.fill(CRGB(255, 0, 0));
}

// ── AnimTurnSignal ───────────────────────────────────────────────────────────
// Sequential amber sweep along the brake strip.
// Left side: sweeps from index 0 → N-1 (outboard → inboard).
// Right side: sweeps from index N-1 → 0 (outboard → inboard).
// Each half-period sweeps; the other half is blank.

void AnimTurnSignal::begin(TailLight& side, LightState /*state*/) {
    _startMs = millis();
}

void AnimTurnSignal::update(TailLight& side, LightState /*state*/, unsigned long nowMs) {
    unsigned long phase      = (nowMs - _startMs) % TURN_BLINK_PERIOD_MS;
    unsigned long halfPeriod = TURN_BLINK_PERIOD_MS / 2;

    if (phase >= halfPeriod) {
        side.fill(CRGB::Black);
        return;
    }

    int numLEDs = side.numPixels();
    int step    = static_cast<int>((phase * numLEDs) / halfPeriod);
    step = constrain(step, 0, numLEDs - 1);

    side.fill(CRGB::Black);

    // Left side sweeps forward (0 → step); right side sweeps backward
    bool forward = side.isLeft();
    for (int i = 0; i <= step; i++) {
        int idx = forward ? i : (numLEDs - 1 - i);
        side.setPixel(idx, CRGB(255, 100, 0));  // amber
    }
}

void AnimTurnSignal::end(TailLight& side) {
    side.fill(CRGB::Black);
}

// ── AnimHazard ───────────────────────────────────────────────────────────────
void AnimHazard::begin(TailLight& side, LightState /*state*/) {
    _startMs = millis();
}

void AnimHazard::update(TailLight& side, LightState /*state*/, unsigned long nowMs) {
    unsigned long phase = (nowMs - _startMs) % TURN_BLINK_PERIOD_MS;
    bool on = phase < (TURN_BLINK_PERIOD_MS / 2);
    side.fill(on ? CRGB(255, 100, 0) : CRGB::Black);
}
