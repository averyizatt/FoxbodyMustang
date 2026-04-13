#pragma once

// ---------------------------------------------------------------------------
// inputs.h
// Debounced reader for the four optocoupler channels.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include "config.h"

class Inputs {
public:
    // Call once in setup()
    void begin();

    // Call every loop iteration.  Returns true when any input changed.
    bool update();

    // Debounced state accessors — true means the stock signal is ACTIVE
    bool brake()      const { return _brake;      }
    bool leftTurn()   const { return _leftTurn;   }
    bool rightTurn()  const { return _rightTurn;  }
    bool reverse()    const { return _reverse;    }

private:
    // One debounce tracker per channel
    struct Channel {
        int     pin;
        bool    state      = false;
        bool    lastRaw    = false;
        unsigned long lastChangeMs = 0;
    };

    Channel _channels[4];

    bool _brake      = false;
    bool _leftTurn   = false;
    bool _rightTurn  = false;
    bool _reverse    = false;

    // Debounce a single channel; returns true if state changed
    bool _debounce(Channel& ch);
};
