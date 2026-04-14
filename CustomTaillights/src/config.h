#pragma once

// ---------------------------------------------------------------------------
// config.h
// Board-level pin assignments and LED strip constants.
// All hardware-specific values live here so nothing else needs to change
// when the wiring is revised.
// ---------------------------------------------------------------------------

// ── LED strip sections ───────────────────────────────────────────────────────
// Each taillight has three independently wired WS2812B strip sections.
// Adjust counts to match the number of LEDs you cut per section.
// (Strip spec: 60 LED/m — so 20 LEDs ≈ 33 cm, 30 LEDs ≈ 50 cm, etc.)
static constexpr int LEDS_PER_SECTION_RUNNING = 20;  // running / parking light
static constexpr int LEDS_PER_SECTION_BRAKE   = 30;  // brake + turn-signal
static constexpr int LEDS_PER_SECTION_REVERSE = 15;  // reverse light

// Data pins — left taillight
static constexpr int PIN_LED_LEFT_RUNNING = 4;   // GPIO4
static constexpr int PIN_LED_LEFT_BRAKE   = 5;   // GPIO5
static constexpr int PIN_LED_LEFT_REVERSE = 6;   // GPIO6

// Data pins — right taillight
static constexpr int PIN_LED_RIGHT_RUNNING = 7;  // GPIO7
static constexpr int PIN_LED_RIGHT_BRAKE   = 8;  // GPIO8
static constexpr int PIN_LED_RIGHT_REVERSE = 9;  // GPIO9

// Global brightness (0-255).  Keep well below 255 to limit current draw.
static constexpr uint8_t BRIGHTNESS_DEFAULT = 128;
static constexpr uint8_t BRIGHTNESS_DIM     =  40;  // running-light level

// FastLED colour order for WS2812B
#define LED_COLOR_ORDER GRB
#define LED_CHIPSET     WS2812B

// ── Optocoupler inputs ───────────────────────────────────────────────────────
// 4-channel optocoupler isolates stock 12 V signals from the ESP32.
// Output pulls GPIO LOW when stock signal is ON → use INPUT_PULLUP.
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
