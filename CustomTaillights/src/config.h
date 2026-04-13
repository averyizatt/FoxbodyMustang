#pragma once

// ---------------------------------------------------------------------------
// config.h
// Board-level pin assignments and LED matrix constants.
// All hardware-specific values live here so nothing else needs to change
// when the wiring is revised.
// ---------------------------------------------------------------------------

// ── LED matrix ──────────────────────────────────────────────────────────────
// Two 8 × 32 WS2812B panels (256 LEDs each), one per tail-light side.
static constexpr int  MATRIX_ROWS   = 8;
static constexpr int  MATRIX_COLS   = 32;
static constexpr int  LEDS_PER_SIDE = MATRIX_ROWS * MATRIX_COLS; // 256

// Data pins for each side
static constexpr int  PIN_LED_LEFT  = 4;   // GPIO4  → left  panel DIN
static constexpr int  PIN_LED_RIGHT = 5;   // GPIO5  → right panel DIN

// Global brightness (0-255).  Keep well below 255 to limit current draw.
static constexpr uint8_t BRIGHTNESS_DEFAULT = 128;
static constexpr uint8_t BRIGHTNESS_DIM     =  40;  // running-light level

// FastLED colour order for these panels
#define LED_COLOR_ORDER GRB
#define LED_CHIPSET     WS2812B

// ── Optocoupler inputs ───────────────────────────────────────────────────────
// The 4-channel optocoupler isolates the stock 12 V signals from the ESP32.
// Output side of each coupler pulls the GPIO LOW when the stock signal is ON.
// Define INPUT_PULLUP below so the lines are HIGH at rest.
static constexpr int PIN_OPT_BRAKE      = 14;  // GPIO14 — brake
static constexpr int PIN_OPT_LEFT_TURN  = 15;  // GPIO15 — left  turn
static constexpr int PIN_OPT_RIGHT_TURN = 16;  // GPIO16 — right turn
static constexpr int PIN_OPT_REVERSE    = 17;  // GPIO17 — reverse

// Logic level when the stock signal is ACTIVE (optocoupler pulls low)
static constexpr int OPT_ACTIVE_LEVEL = LOW;

// Debounce time in milliseconds
static constexpr unsigned long DEBOUNCE_MS = 20;

// ── Animation timing ─────────────────────────────────────────────────────────
// How often the main loop calls the active animation's update() method.
static constexpr unsigned long FRAME_INTERVAL_MS = 20;   // ~50 fps

// Turn-signal blink period (total on+off cycle), milliseconds
static constexpr unsigned long TURN_BLINK_PERIOD_MS = 600;
