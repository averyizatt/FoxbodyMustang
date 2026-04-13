#pragma once

// ---------------------------------------------------------------------------
// states.h
// Enum that represents every possible taillight state the system can be in,
// plus a helper that derives the current state from the raw input flags.
// ---------------------------------------------------------------------------

// Each bit represents one active stock signal.
enum class LightState : uint8_t {
    OFF         = 0b0000,  // ignition on but nothing active
    BRAKE       = 0b0001,
    LEFT_TURN   = 0b0010,
    RIGHT_TURN  = 0b0100,
    REVERSE     = 0b1000,

    // Combinations the animation layer may treat differently
    BRAKE_LEFT  = 0b0011,  // brake + left  turn
    BRAKE_RIGHT = 0b0101,  // brake + right turn
    HAZARD      = 0b0110,  // left + right turn simultaneously
};

// ---------------------------------------------------------------------------
// resolveLightState()
// Combine the four raw boolean inputs into the matching LightState.
// Call this once per frame after debouncing.
// ---------------------------------------------------------------------------
inline LightState resolveLightState(bool brake, bool leftTurn,
                                    bool rightTurn, bool reverse)
{
    // Hazard: both turn signals active at the same time (check first)
    if (leftTurn && rightTurn) {
        return LightState::HAZARD;
    }

    uint8_t bits = (brake     ? 0b0001 : 0)
                 | (leftTurn  ? 0b0010 : 0)
                 | (rightTurn ? 0b0100 : 0)
                 | (reverse   ? 0b1000 : 0);

    switch (bits) {
        case 0b0001: return LightState::BRAKE;
        case 0b0010: return LightState::LEFT_TURN;
        case 0b0100: return LightState::RIGHT_TURN;
        case 0b1000: return LightState::REVERSE;
        case 0b0011: return LightState::BRAKE_LEFT;
        case 0b0101: return LightState::BRAKE_RIGHT;
        default:     return LightState::OFF;
    }
}
