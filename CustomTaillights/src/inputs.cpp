// ---------------------------------------------------------------------------
// inputs.cpp
// ---------------------------------------------------------------------------

#include "inputs.h"

void Inputs::begin() {
    _channels[0] = { PIN_OPT_BRAKE,      false, false, 0 };
    _channels[1] = { PIN_OPT_LEFT_TURN,  false, false, 0 };
    _channels[2] = { PIN_OPT_RIGHT_TURN, false, false, 0 };
    _channels[3] = { PIN_OPT_REVERSE,    false, false, 0 };

    for (auto& ch : _channels) {
        pinMode(ch.pin, INPUT_PULLUP);
    }
}

bool Inputs::update() {
    bool changed = false;

    changed |= _debounce(_channels[0]);
    changed |= _debounce(_channels[1]);
    changed |= _debounce(_channels[2]);
    changed |= _debounce(_channels[3]);

    _brake     = _channels[0].state;
    _leftTurn  = _channels[1].state;
    _rightTurn = _channels[2].state;
    _reverse   = _channels[3].state;

    return changed;
}

bool Inputs::_debounce(Channel& ch) {
    bool raw = (digitalRead(ch.pin) == OPT_ACTIVE_LEVEL);

    if (raw != ch.lastRaw) {
        ch.lastRaw      = raw;
        ch.lastChangeMs = millis();
    }

    if ((millis() - ch.lastChangeMs) >= DEBOUNCE_MS && raw != ch.state) {
        ch.state = raw;
        return true;
    }

    return false;
}
