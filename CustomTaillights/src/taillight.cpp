// ---------------------------------------------------------------------------
// taillight.cpp
// ---------------------------------------------------------------------------

#include "taillight.h"

void TailLight::begin() {
    fill(CRGB::Black);
    _currentState = LightState::OFF;
    _currentAnim  = AnimationRegistry::get(LightState::OFF, _isLeft);
    if (_currentAnim) _currentAnim->begin(*this, _currentState);
}

void TailLight::update(LightState state, unsigned long nowMs) {
    // On state change, swap to the new animation
    if (state != _currentState) {
        if (_currentAnim) _currentAnim->end(*this);

        _currentState = state;
        _currentAnim  = AnimationRegistry::get(state, _isLeft);

        if (_currentAnim) _currentAnim->begin(*this, _currentState);
    }

    if (_currentAnim) {
        _currentAnim->update(*this, _currentState, nowMs);
    }
}

void TailLight::fill(CRGB colour) {
    for (int i = 0; i < LEDS_PER_SIDE; i++) {
        _pixels[i] = colour;
    }
}

void TailLight::setPixel(int row, int col, CRGB colour) {
    if (row < 0 || row >= MATRIX_ROWS) return;
    if (col < 0 || col >= MATRIX_COLS) return;
    _pixels[_index(row, col)] = colour;
}

int TailLight::_index(int row, int col) const {
    // Serpentine layout: even rows run left→right, odd rows run right→left
    if (row % 2 == 0) {
        return row * MATRIX_COLS + col;
    } else {
        return row * MATRIX_COLS + (MATRIX_COLS - 1 - col);
    }
}
