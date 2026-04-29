# Custom Taillights

Custom taillight controller for the Foxbody Mustang using an **ESP32-S3** and two **8×32 WS2812B LED matrix panels** (256 pixels each, one per side).  
Stock 12 V signals are read through a **4-channel optocoupler** for full isolation.  
Built with **PlatformIO**, **FastLED**, and **ArduinoJson**.

---

## Hardware

| Item | Detail |
|------|--------|
| MCU | ESP32-S3 DevKitC-1 |
| LED panels | 8×32 WS2812B (256 px), ×2 |
| Input isolation | 4-channel optocoupler module |
| Power | 5 V (LED panels) + 3.3 V (ESP32) |

### Pin assignments

| Signal | GPIO |
|--------|------|
| Left panel DIN | 4 |
| Right panel DIN | 5 |
| Brake (opto CH1) | 14 |
| Left turn (opto CH2) | 15 |
| Right turn (opto CH3) | 16 |
| Reverse (opto CH4) | 17 |

All GPIO assignments and timing constants are in `src/config.h`.

---

## Project structure

```
CustomTaillights/
├── platformio.ini          # PlatformIO build config (ESP32-S3, FastLED, ArduinoJson)
└── src/
    ├── main.cpp            # setup() / loop() entry point
    ├── config.h            # Pin defs, matrix size, default constants
    ├── settings.h / .cpp   # Runtime settings struct + NVS (Preferences) persistence
    ├── wifi_server.h / .cpp # WiFi (AP/STA) + HTTP settings server
    ├── states.h            # LightState enum + resolveLightState()
    ├── inputs.h / .cpp     # Debounced 4-channel optocoupler reader
    ├── taillight.h / .cpp  # Per-side LED panel controller
    └── animations.h / .cpp # Animation base class + built-in animations
```

---

## WiFi settings page

The ESP32-S3 hosts a **mobile-optimised web UI** at `http://192.168.4.1` (default AP mode).

### First connection

1. On your phone or laptop, connect to the WiFi network **`Foxbody-Taillights`**  
   Password: **`mustang87`**
2. Open a browser and navigate to **`http://192.168.4.1`**

### What you can configure

| Section | Controls |
|---------|----------|
| **Brightness** | Main LED intensity (10–255) and running-light dim level (5–100) |
| **Colors** | Brake, turn-signal/hazard, and reverse light colors (full color picker) |
| **Timing** | Turn-signal blink period (200–1500 ms) and animation frame interval (10–100 ms) |
| **Preview** | Trigger any light state for 3 seconds to test colors live |
| **Network** | Switch between AP mode (hotspot) and Station mode (connect to home WiFi) |

All settings are saved to NVS (flash) and survive reboots.

### HTTP API (expandable)

| Method | Path | Description |
|--------|------|-------------|
| `GET`  | `/` | Serve the settings web page |
| `GET`  | `/api/settings` | Return current settings as JSON |
| `POST` | `/api/settings` | Update settings (JSON body); applies brightness immediately |
| `POST` | `/api/reset` | Reset all settings to factory defaults |
| `POST` | `/api/reboot` | Restart the ESP32 |
| `POST` | `/api/preview` | Temporarily override light state for 3 s (`{"state":"brake"}`) |

---

## Light states & built-in animations

| State | Left panel | Right panel |
|-------|-----------|------------|
| OFF | black | black |
| BRAKE | solid red* | solid red* |
| LEFT_TURN | amber* column sweep → | black |
| RIGHT_TURN | black | ← amber* column sweep |
| REVERSE | white* | white* |
| BRAKE_LEFT | amber sweep | solid red |
| BRAKE_RIGHT | solid red | amber sweep |
| HAZARD | amber* flash | amber* flash |

\* Color configurable via the web UI

---

## Adding a new animation

1. Subclass `Animation` in `animations.h` / `animations.cpp`:

```cpp
class AnimMyEffect : public Animation {
public:
    void begin(TailLight& side, LightState state) override;   // optional
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
    void end(TailLight& side) override;                        // optional
};
```

2. Register it in `AnimationRegistry::get()` for the desired `LightState`.
3. Use `g_settings` (from `settings.h`) for colors and timing so your animation
   respects the web-UI controls automatically.

---

## Building & flashing

```bash
# Install PlatformIO CLI first, then:
cd CustomTaillights
pio run                   # compile
pio run --target upload   # flash over USB
pio device monitor        # open serial monitor
```
