#pragma once

// ---------------------------------------------------------------------------
// animations.h
// Base class for brake-section animations and a global registry.
//
// Animations operate exclusively on the TailLight's BRAKE section via the
// fill() and setPixel() helpers.  The RUNNING and REVERSE sections are
// driven directly by TailLight::update().
//
// To add a new animation:
//   1. Create a class that inherits Animation.
//   2. Override begin(), update(), and (optionally) end().
//   3. Register an instance in AnimationRegistry::get() inside animations.cpp.
// ---------------------------------------------------------------------------

#include <FastLED.h>
#include "states.h"

// Forward declaration
class TailLight;

// ---------------------------------------------------------------------------
// Animation — abstract base
// ---------------------------------------------------------------------------
class Animation {
public:
    virtual ~Animation() = default;

    // Called once when this animation becomes active.
    virtual void begin(TailLight& side, LightState state) {}

    // Called every FRAME_INTERVAL_MS while this animation is active.
    // Write LED colours into the TailLight's brake-section pixel buffer.
    virtual void update(TailLight& side, LightState state, unsigned long nowMs) = 0;

    // Called once when this animation is deactivated (state changed).
    virtual void end(TailLight& side) {}
};

// ---------------------------------------------------------------------------
// Built-in animations (defined in animations.cpp)
// ---------------------------------------------------------------------------

// Brake section off (black)
class AnimOff : public Animation {
public:
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
};

// Solid bright red — brake
class AnimBrake : public Animation {
public:
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
};

// Sequential amber sweep along the brake strip — turn signal
class AnimTurnSignal : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
    void end(TailLight& side) override;
private:
    unsigned long _startMs = 0;
};

// Simultaneous amber flash — hazard
class AnimHazard : public Animation {
public:
    void begin(TailLight& side, LightState state) override;
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
private:
    unsigned long _startMs = 0;
};

// ---------------------------------------------------------------------------
// AnimationRegistry
// Maps a LightState to the correct Animation for the BRAKE section.
// ---------------------------------------------------------------------------
class AnimationRegistry {
public:
    // Called once in setup() to create all animation instances.
    static void init();

    // Return the animation that should play for `state` on the given side.
    static Animation* get(LightState state, bool isLeft);

private:
    static AnimOff        _off;
    static AnimBrake      _brake;
    static AnimTurnSignal _turnLeft;
    static AnimTurnSignal _turnRight;
    static AnimHazard     _hazard;
};
