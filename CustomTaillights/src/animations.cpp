// ---------------------------------------------------------------------------
// animations.cpp
// Concrete animation implementations.
// ---------------------------------------------------------------------------

#include "animations.h"
#include "taillight.h"
#include "config.h"
#include "settings.h"

// ── Static instance definitions ─────────────────────────────────────────────
AnimOff          AnimationRegistry::_off;
AnimRunning      AnimationRegistry::_running;
AnimRunBreathe   AnimationRegistry::_runBreathe;
AnimBrake        AnimationRegistry::_brake;
AnimPulse        AnimationRegistry::_brakePulse   { ColorSource::BRAKE   };
AnimCenterOut    AnimationRegistry::_brakeCenterOut;
AnimStrobe       AnimationRegistry::_brakeStrobe;
AnimTurnSignal   AnimationRegistry::_turnLeft;
AnimTurnSignal   AnimationRegistry::_turnRight;
AnimTurnFlash    AnimationRegistry::_turnFlashL;
AnimTurnFlash    AnimationRegistry::_turnFlashR;
AnimTurnChase    AnimationRegistry::_turnChaseL;
AnimTurnChase    AnimationRegistry::_turnChaseR;
AnimTurnBounce   AnimationRegistry::_turnBounceL;
AnimTurnBounce   AnimationRegistry::_turnBounceR;
AnimReverse      AnimationRegistry::_reverse;
AnimPulse        AnimationRegistry::_reversePulse { ColorSource::REVERSE };
AnimHazard       AnimationRegistry::_hazard;

// ---------------------------------------------------------------------------
void AnimationRegistry::init() {
    // Nothing to allocate dynamically — all instances are static.
}

// ---------------------------------------------------------------------------
// Lens preset helper
// Returns true when (row, col) should be illuminated for the active preset.
// Pixels where this returns false are left black, simulating the chrome trim
// gaps present on different Foxbody taillight housing styles.
//
// Column layout (0 = leftmost, 31 = rightmost on each panel):
//   0  Full panel     — all 256 pixels active
//   1  GT Cheese Grater — three sections separated by trim gaps
//                         [0-9]  gap[10-11]  [12-20]  gap[21-22]  [23-31]
//   2  LX / Base      — two wide sections with a center gap
//                         [0-13]  gap[14-17]  [18-31]
//   3  Cobra Bar      — horizontal center bar, rows 2-5 only (4 of 8 rows)
// ---------------------------------------------------------------------------
static bool lensActive(int row, int col) {
    switch (g_settings.lens_preset) {
        case 1:   // GT Cheese Grater
            return (col <= 9) || (col >= 12 && col <= 20) || (col >= 23);
        case 2:   // LX / Base
            return (col <= 13) || (col >= 18);
        case 3:   // Cobra Bar
            return (row >= 2 && row <= 5);
        default:  // Full panel
            return true;
    }
}

// Fill the panel with `color` respecting the active lens preset.
// For black / clear operations use side.fill(CRGB::Black) directly.
static void fillLens(TailLight& side, CRGB color) {
    if (g_settings.lens_preset == 0) {
        side.fill(color);
        return;
    }
    side.fill(CRGB::Black);
    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            if (lensActive(r, c))
                side.setPixel(r, c, color);
        }
    }
}

// ---------------------------------------------------------------------------
// AnimationRegistry — internal helpers
// ---------------------------------------------------------------------------
Animation* AnimationRegistry::_getBrakeAnim() {
    switch (g_settings.brake_anim) {
        case 1:  return &_brakePulse;
        case 2:  return &_brakeCenterOut;
        case 3:  return &_brakeStrobe;
        default: return &_brake;
    }
}

Animation* AnimationRegistry::_getTurnAnim(bool isLeft) {
    switch (g_settings.turn_anim) {
        case 1:  return isLeft ? &_turnFlashL  : &_turnFlashR;
        case 2:  return isLeft ? &_turnChaseL  : &_turnChaseR;
        case 3:  return isLeft ? &_turnBounceL : &_turnBounceR;
        default: return isLeft ? &_turnLeft    : &_turnRight;
    }
}

Animation* AnimationRegistry::_getReverseAnim() {
    return (g_settings.reverse_anim == 1) ? (Animation*)&_reversePulse : &_reverse;
}

Animation* AnimationRegistry::_getRunningAnim() {
    return (g_settings.run_anim == 1) ? (Animation*)&_runBreathe : &_running;
}

// ---------------------------------------------------------------------------
Animation* AnimationRegistry::get(LightState state, bool isLeft) {
    switch (state) {
        case LightState::BRAKE:       return _getBrakeAnim();
        case LightState::LEFT_TURN:   return isLeft ? _getTurnAnim(true)  : &_off;
        case LightState::RIGHT_TURN:  return isLeft ? &_off : _getTurnAnim(false);
        case LightState::REVERSE:     return _getReverseAnim();
        case LightState::BRAKE_LEFT:  return isLeft ? _getTurnAnim(true)  : _getBrakeAnim();
        case LightState::BRAKE_RIGHT: return isLeft ? _getBrakeAnim()     : _getTurnAnim(false);
        case LightState::HAZARD:      return &_hazard;
        case LightState::OFF:
        default:                      return _getRunningAnim();
    }
}

// ===========================================================================
// ── Timing constants for parameterised animations ────────────────────────────
// ===========================================================================
static constexpr uint8_t  BREATHE_SPEED_DIV  = 10;   // ms/step → ~2.56 s/breath
static constexpr uint8_t  MIN_BREATHE_LEVEL  =  8;   // absolute floor for breathe low
static constexpr uint8_t  PULSE_SPEED_DIV    =  8;   // ms/step → ~2.05 s/cycle
static constexpr uint8_t  PULSE_MIN_SCALE    = 128;  // keeps pulse ≥ 50 % bright
static constexpr uint16_t STROBE_PERIOD_MS   = 125;  // ~8 Hz strobe period
static constexpr uint16_t STROBE_ON_MS       =  62;  // on-time within period

// ===========================================================================
// ── AnimOff ──────────────────────────────────────────────────────────────────
// ===========================================================================
void AnimOff::update(TailLight& side, LightState, unsigned long) {
    side.fill(CRGB::Black);
}

// ===========================================================================
// ── AnimRunning ──────────────────────────────────────────────────────────────
// ===========================================================================
void AnimRunning::update(TailLight& side, LightState, unsigned long) {
    fillLens(side, CRGB(g_settings.brightness_dim, 0, 0));
}

// ===========================================================================
// ── AnimRunBreathe ───────────────────────────────────────────────────────────
// Gentle breathing effect for running lights using FastLED sin8().
// ===========================================================================
void AnimRunBreathe::begin(TailLight&, LightState) {
    _startMs = millis();
}

void AnimRunBreathe::update(TailLight& side, LightState, unsigned long nowMs) {
    // sin8(t) → 0-255 over one 256-step cycle; ~2.56 s per breath at 10 ms/step
    uint8_t t     = (uint8_t)((nowMs - _startMs) / BREATHE_SPEED_DIV);
    uint8_t scale = sin8(t);  // 0-255
    uint8_t hi    = g_settings.brightness_dim;
    uint8_t lo    = max((uint8_t)MIN_BREATHE_LEVEL, (uint8_t)(hi / 4));
    if (hi < lo) hi = lo;    // guard against very-low brightness_dim setting
    uint8_t level = (uint8_t)(lo + ((uint16_t)(hi - lo) * scale) / 255);
    fillLens(side, CRGB(level, 0, 0));
}

// ===========================================================================
// ── AnimBrake ────────────────────────────────────────────────────────────────
// ===========================================================================
void AnimBrake::update(TailLight& side, LightState, unsigned long) {
    fillLens(side, CRGB(g_settings.brake_r, g_settings.brake_g, g_settings.brake_b));
}

// ===========================================================================
// ── AnimPulse ────────────────────────────────────────────────────────────────
// Breathes between 50 % and 100 % of the source color so the light is never
// fully dark while the state is active (brake / reverse).
// ===========================================================================
CRGB AnimPulse::_color() const {
    switch (_src) {
        case ColorSource::BRAKE:   return CRGB(g_settings.brake_r,   g_settings.brake_g,   g_settings.brake_b);
        case ColorSource::REVERSE: return CRGB(g_settings.reverse_r, g_settings.reverse_g, g_settings.reverse_b);
        default:                   return CRGB::Black;
    }
}

void AnimPulse::begin(TailLight&, LightState) {
    _startMs = millis();
}

void AnimPulse::update(TailLight& side, LightState, unsigned long nowMs) {
    // ~2 s period; scale oscillates PULSE_MIN_SCALE-255 so light stays ≥ 50 %
    uint8_t t     = (uint8_t)((nowMs - _startMs) / PULSE_SPEED_DIV);
    uint8_t wave  = sin8(t);                                    // 0-255
    uint8_t scale = (uint8_t)(PULSE_MIN_SCALE + wave / 2);     // 128-255
    CRGB    c     = _color();
    c.nscale8(scale);
    fillLens(side, c);
}

// ===========================================================================
// ── AnimCenterOut ────────────────────────────────────────────────────────────
// Fills from the center column outward over 500 ms then holds solid.
// ===========================================================================
void AnimCenterOut::begin(TailLight&, LightState) {
    _startMs = millis();
}

void AnimCenterOut::update(TailLight& side, LightState, unsigned long nowMs) {
    const unsigned long EXPAND_MS = 500UL;
    unsigned long elapsed = nowMs - _startMs;

    int reach = (elapsed >= EXPAND_MS)
                    ? (MATRIX_COLS / 2 + 1)
                    : (int)((elapsed * (MATRIX_COLS / 2 + 1)) / EXPAND_MS);
    reach = constrain(reach, 0, MATRIX_COLS / 2 + 1);

    CRGB c(g_settings.brake_r, g_settings.brake_g, g_settings.brake_b);
    side.fill(CRGB::Black);
    int center = MATRIX_COLS / 2;
    for (int col = center - reach; col <= center + reach; col++) {
        if (col < 0 || col >= MATRIX_COLS) continue;
        for (int row = 0; row < MATRIX_ROWS; row++) {
            if (lensActive(row, col))
                side.setPixel(row, col, c);
        }
    }
}

// ===========================================================================
// ── AnimStrobe ───────────────────────────────────────────────────────────────
// ~8 Hz strobe: 62 ms on, 63 ms off (125 ms period).
// ===========================================================================
void AnimStrobe::begin(TailLight&, LightState) {
    _startMs = millis();
}

void AnimStrobe::update(TailLight& side, LightState, unsigned long nowMs) {
    unsigned long phase = (nowMs - _startMs) % STROBE_PERIOD_MS;
    if (phase < STROBE_ON_MS) {
        fillLens(side, CRGB(g_settings.brake_r, g_settings.brake_g, g_settings.brake_b));
    } else {
        side.fill(CRGB::Black);
    }
}

// ===========================================================================
// ── AnimTurnSignal ───────────────────────────────────────────────────────────
// Classic single-column sequential sweep.
// ===========================================================================
void AnimTurnSignal::begin(TailLight&, LightState) {
    _startMs = millis();
    _step    = 0;
}

void AnimTurnSignal::update(TailLight& side, LightState, unsigned long nowMs) {
    unsigned long halfPeriod = g_settings.turn_blink_ms / 2;
    unsigned long phase      = (nowMs - _startMs) % g_settings.turn_blink_ms;

    if (phase >= halfPeriod) {
        side.fill(CRGB::Black);
        _step = 0;
        return;
    }

    _step = static_cast<int>((phase * MATRIX_COLS) / halfPeriod);
    _step = constrain(_step, 0, MATRIX_COLS - 1);

    side.fill(CRGB::Black);
    bool sweepForward = side.isLeft();
    CRGB c(g_settings.turn_r, g_settings.turn_g, g_settings.turn_b);

    for (int col = 0; col <= _step; col++) {
        int ci = sweepForward ? col : (MATRIX_COLS - 1 - col);
        for (int row = 0; row < MATRIX_ROWS; row++) {
            if (lensActive(row, ci))
                side.setPixel(row, ci, c);
        }
    }
}

void AnimTurnSignal::end(TailLight& side) {
    side.fill(CRGB::Black);
}

// ===========================================================================
// ── AnimTurnFlash ─────────────────────────────────────────────────────────────
// Simple whole-panel on/off flash (no directional sweep).
// ===========================================================================
void AnimTurnFlash::begin(TailLight&, LightState) {
    _startMs = millis();
}

void AnimTurnFlash::update(TailLight& side, LightState, unsigned long nowMs) {
    unsigned long phase = (nowMs - _startMs) % g_settings.turn_blink_ms;
    bool on = phase < (g_settings.turn_blink_ms / 2);
    if (on) {
        fillLens(side, CRGB(g_settings.turn_r, g_settings.turn_g, g_settings.turn_b));
    } else {
        side.fill(CRGB::Black);
    }
}

// ===========================================================================
// ── AnimTurnChase ─────────────────────────────────────────────────────────────
// Group-sequential: 8-column groups sweep across the panel one at a time,
// each group stepping on in sequence before the panel blanks and resets.
// ===========================================================================
void AnimTurnChase::begin(TailLight&, LightState) {
    _startMs = millis();
}

void AnimTurnChase::update(TailLight& side, LightState, unsigned long nowMs) {
    const int  NUM_GROUPS    = 4;
    const int  COLS_PER_GRP  = MATRIX_COLS / NUM_GROUPS;  // 8
    unsigned long halfPeriod = g_settings.turn_blink_ms / 2;
    unsigned long phase      = (nowMs - _startMs) % g_settings.turn_blink_ms;

    if (phase >= halfPeriod) {
        side.fill(CRGB::Black);
        return;
    }

    int group = (int)((phase * NUM_GROUPS) / halfPeriod);
    group = constrain(group, 0, NUM_GROUPS - 1);

    bool sweepForward = side.isLeft();
    side.fill(CRGB::Black);
    CRGB c(g_settings.turn_r, g_settings.turn_g, g_settings.turn_b);

    for (int g = 0; g <= group; g++) {
        for (int offset = 0; offset < COLS_PER_GRP; offset++) {
            int col = sweepForward
                        ? (g * COLS_PER_GRP + offset)
                        : (MATRIX_COLS - 1 - g * COLS_PER_GRP - offset);
            if (col < 0 || col >= MATRIX_COLS) continue;
            for (int row = 0; row < MATRIX_ROWS; row++) {
                if (lensActive(row, col))
                    side.setPixel(row, col, c);
            }
        }
    }
}

// ===========================================================================
// ── AnimTurnBounce ────────────────────────────────────────────────────────────
// Knight Rider style: a 4-wide beam bounces during the on-half of the blink
// cycle, then blanks.  Direction-aware so it naturally reads inward→outward.
// ===========================================================================
void AnimTurnBounce::begin(TailLight&, LightState) {
    _startMs = millis();
}

void AnimTurnBounce::update(TailLight& side, LightState, unsigned long nowMs) {
    const int  BEAM_WIDTH    = 4;
    const int  MAX_POS       = MATRIX_COLS - BEAM_WIDTH;   // 28
    unsigned long halfPeriod = g_settings.turn_blink_ms / 2;
    unsigned long phase      = (nowMs - _startMs) % g_settings.turn_blink_ms;

    if (phase >= halfPeriod) {
        side.fill(CRGB::Black);
        return;
    }

    // Triangle wave: 0 → MAX_POS → 0 over halfPeriod
    int rawPos;
    if (phase < halfPeriod / 2) {
        rawPos = (int)((phase * MAX_POS * 2) / halfPeriod);
    } else {
        rawPos = (int)(((halfPeriod - phase) * MAX_POS * 2) / halfPeriod);
    }
    rawPos = constrain(rawPos, 0, MAX_POS);

    // Mirror direction so beam reads inner→outer on each side
    int beamStart = side.isLeft() ? rawPos : (MATRIX_COLS - 1 - rawPos - BEAM_WIDTH + 1);

    side.fill(CRGB::Black);
    CRGB c(g_settings.turn_r, g_settings.turn_g, g_settings.turn_b);
    for (int col = beamStart; col < beamStart + BEAM_WIDTH; col++) {
        if (col < 0 || col >= MATRIX_COLS) continue;
        for (int row = 0; row < MATRIX_ROWS; row++) {
            if (lensActive(row, col))
                side.setPixel(row, col, c);
        }
    }
}

// ===========================================================================
// ── AnimReverse ──────────────────────────────────────────────────────────────
// ===========================================================================
void AnimReverse::update(TailLight& side, LightState, unsigned long) {
    fillLens(side, CRGB(g_settings.reverse_r, g_settings.reverse_g, g_settings.reverse_b));
}

// ===========================================================================
// ── AnimHazard ───────────────────────────────────────────────────────────────
// ===========================================================================
void AnimHazard::begin(TailLight&, LightState) {
    _startMs = millis();
}

void AnimHazard::update(TailLight& side, LightState, unsigned long nowMs) {
    unsigned long phase = (nowMs - _startMs) % g_settings.turn_blink_ms;
    bool on = phase < (g_settings.turn_blink_ms / 2);
    if (on) {
        fillLens(side, CRGB(g_settings.turn_r, g_settings.turn_g, g_settings.turn_b));
    } else {
        side.fill(CRGB::Black);
    }
}
