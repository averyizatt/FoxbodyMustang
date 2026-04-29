#pragma once

// ---------------------------------------------------------------------------
// animations.h
// Base class for all taillight animations and a global registry.
//
// To add a new animation:
//   1. Create a class that inherits Animation.
//   2. Override begin(), update(), and (optionally) end().
//   3. Register an instance in AnimationRegistry and add it to get().
// ---------------------------------------------------------------------------

#include <FastLED.h>
#include "states.h"

// Forward declaration
class TailLight;

// ---------------------------------------------------------------------------
// ColorSource — selects which g_settings color a parameterised animation uses
// ---------------------------------------------------------------------------
enum class ColorSource : uint8_t { BRAKE, TURN, REVERSE };

// ---------------------------------------------------------------------------
// Animation — abstract base
// ---------------------------------------------------------------------------
class Animation {
public:
    virtual ~Animation() = default;

    // Called once when this animation becomes active.
    virtual void begin(TailLight& side, LightState state) {}

    // Called every frame while this animation is active.
    virtual void update(TailLight& side, LightState state, unsigned long nowMs) = 0;

    // Called once when this animation is deactivated (state changed).
    virtual void end(TailLight& side) {}
};

// ---------------------------------------------------------------------------
// ── OFF ──────────────────────────────────────────────────────────────────────
// ---------------------------------------------------------------------------
class AnimOff : public Animation {
public:
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
};

// ---------------------------------------------------------------------------
// ── RUNNING LIGHTS ───────────────────────────────────────────────────────────
// ---------------------------------------------------------------------------

// Dim solid red — parking / running lights
class AnimRunning : public Animation {
public:
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
};

// Gentle breathing / breathe — running light alternative
class AnimRunBreathe : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    unsigned long _startMs = 0;
};

// ---------------------------------------------------------------------------
// ── BRAKE ────────────────────────────────────────────────────────────────────
// ---------------------------------------------------------------------------

// Solid bright fill using brake color
class AnimBrake : public Animation {
public:
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
};

// Breathing pulse — uses ColorSource to select brake or reverse color
class AnimPulse : public Animation {
public:
    explicit AnimPulse(ColorSource src) : _src(src) {}
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    ColorSource   _src;
    unsigned long _startMs = 0;
    CRGB _color() const;
};

// Center-to-edges fill then holds solid — brake entry animation
class AnimCenterOut : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    unsigned long _startMs = 0;
};

// Rapid ~8 Hz strobe — attention-grabbing brake effect
class AnimStrobe : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    unsigned long _startMs = 0;
};

// ---------------------------------------------------------------------------
// ── TURN SIGNAL ──────────────────────────────────────────────────────────────
// ---------------------------------------------------------------------------

// Classic sequential column sweep (default)
class AnimTurnSignal : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
    void end(TailLight& side) override;
private:
    unsigned long _startMs = 0;
    int           _step    = 0;
};

// Simple whole-panel flash (no sweep)
class AnimTurnFlash : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    unsigned long _startMs = 0;
};

// Group-sequential: 4 column-wide groups light up one at a time
class AnimTurnChase : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    unsigned long _startMs = 0;
};

// Knight Rider style — 4-wide beam bounces across the panel
class AnimTurnBounce : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    unsigned long _startMs = 0;
};

// ---------------------------------------------------------------------------
// ── REVERSE ──────────────────────────────────────────────────────────────────
// ---------------------------------------------------------------------------

// Solid white (default)
class AnimReverse : public Animation {
public:
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
};

// ---------------------------------------------------------------------------
// ── HAZARD ───────────────────────────────────────────────────────────────────
// ---------------------------------------------------------------------------

// Simultaneous amber flash on both sides
class AnimHazard : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    unsigned long _startMs = 0;
};

// ---------------------------------------------------------------------------
// AnimationRegistry
// Maps a LightState + side to the correct Animation, respecting g_settings.
// ---------------------------------------------------------------------------
class AnimationRegistry {
public:
    // Called once in setup() to initialise animation instances.
    static void init();

    // Return the animation for `state` on the given side.
    static Animation* get(LightState state, bool isLeft);

private:
    // ── Off / running ──
    static AnimOff          _off;
    static AnimRunning      _running;
    static AnimRunBreathe   _runBreathe;

    // ── Brake ──
    static AnimBrake        _brake;
    static AnimPulse        _brakePulse;
    static AnimCenterOut    _brakeCenterOut;
    static AnimStrobe       _brakeStrobe;

    // ── Turn signal (separate L/R instances for independent timing) ──
    static AnimTurnSignal   _turnLeft;
    static AnimTurnSignal   _turnRight;
    static AnimTurnFlash    _turnFlashL;
    static AnimTurnFlash    _turnFlashR;
    static AnimTurnChase    _turnChaseL;
    static AnimTurnChase    _turnChaseR;
    static AnimTurnBounce   _turnBounceL;
    static AnimTurnBounce   _turnBounceR;

    // ── Reverse ──
    static AnimReverse      _reverse;
    static AnimPulse        _reversePulse;

    // ── Hazard ──
    static AnimHazard       _hazard;

    // Internal helpers
    static Animation* _getBrakeAnim();
    static Animation* _getTurnAnim(bool isLeft);
    static Animation* _getReverseAnim();
    static Animation* _getRunningAnim();
};
