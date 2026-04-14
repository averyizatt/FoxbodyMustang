# Custom Taillights

Custom taillight controller for the Foxbody Mustang using an **ESP32-S3** and **WS2812B LED strips** (60 LED/m, DC 5V, IP30).  
Each taillight has three independently wired and controllable strip sections: **running**, **brake/turn**, and **reverse**.  
Stock 12 V signals are read through a **4-channel optocoupler** for full isolation.  
Built with **PlatformIO** and **FastLED**.

---

## Hardware

| Item | Detail |
|------|--------|
| MCU | ESP32-S3 DevKitC-1 |
| LED strips | WS2812B 60 LED/m, DC 5V (e.g. 16.4 ft / 300 LED roll, black PCB, IP30) |
| Strip sections per side | 3 (running, brake/turn, reverse) |
| Input isolation | 4-channel optocoupler module |
| Power | 5 V (LED strips) + 3.3 V (ESP32) |

### Pin assignments

| Signal | GPIO |
|--------|------|
| Left  running section DIN | 4 |
| Left  brake section DIN   | 5 |
| Left  reverse section DIN | 6 |
| Right running section DIN | 7 |
| Right brake section DIN   | 8 |
| Right reverse section DIN | 9 |
| Brake (opto CH1)      | 14 |
| Left turn (opto CH2)  | 15 |
| Right turn (opto CH3) | 16 |
| Reverse (opto CH4)    | 17 |

All GPIO assignments, LED counts, and timing constants are in `src/config.h`.

---

## Strip section layout (per side)

| Section | Default LED count | Function |
|---------|:-----------------:|------------------------------------------|
| RUNNING | 20 | Dim red running / parking light |
| BRAKE   | 30 | Bright red brake + amber turn-signal |
| REVERSE | 15 | White reverse light |

Adjust `LEDS_PER_SECTION_RUNNING`, `LEDS_PER_SECTION_BRAKE`, and `LEDS_PER_SECTION_REVERSE` in `config.h` to match the lengths you cut from the roll (60 LED/m → 10 LEDs ≈ 17 cm).

---

## Project structure

```
CustomTaillights/
├── platformio.ini          # PlatformIO build config (ESP32-S3, FastLED)
└── src/
    ├── main.cpp            # setup() / loop() entry point
    ├── config.h            # Pin defs, section sizes, timing constants
    ├── states.h            # LightState enum + resolveLightState()
    ├── inputs.h / .cpp     # Debounced 4-channel optocoupler reader
    ├── taillight.h / .cpp  # Per-side taillight controller (3 sections)
    └── animations.h / .cpp # Animation base class + brake-section animations
```

---

## Section behavior by state

| State | Running section | Brake section | Reverse section |
|-------|:--------------:|:-------------:|:---------------:|
| OFF         | off     | off                              | off   |
| BRAKE       | dim red | solid bright red                 | off   |
| LEFT_TURN   | dim red | left: amber sweep / right: off   | off   |
| RIGHT_TURN  | dim red | left: off / right: amber sweep   | off   |
| REVERSE     | dim red | off                              | white |
| BRAKE_LEFT  | dim red | left: amber sweep / right: red   | off   |
| BRAKE_RIGHT | dim red | left: red / right: amber sweep   | off   |
| HAZARD      | dim red | amber flash (both sides)         | off   |

---

## Adding a new animation

Brake-section animations subclass `Animation` in `animations.h` / `animations.cpp`:

```cpp
class AnimMyEffect : public Animation {
public:
    void begin(TailLight& side, LightState state) override;   // optional
    void update(TailLight& side, LightState state, unsigned long nowMs) override;
    void end(TailLight& side) override;                        // optional
};
```

Register it in `AnimationRegistry::get()` for the desired `LightState`.  
Use `side.fill(colour)` and `side.setPixel(index, colour)` to write to the brake strip (linear index 0 → `side.numPixels()-1`).

---

## Building & flashing

```bash
# Install PlatformIO CLI first, then:
cd CustomTaillights
pio run                   # compile
pio run --target upload   # flash over USB
pio device monitor        # open serial monitor
```
