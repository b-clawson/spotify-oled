# HUB75 to ESP32-S3 Pinout Reference

Quick reference for wiring your 64x64 HUB75 RGB LED matrix to ESP32-S3.

## Pin Mapping Table

| HUB75 Pin | Function | ESP32-S3 GPIO | Wire Color Hint |
|:---------:|:---------|:-------------:|:---------------:|
| **R1** | Red Upper | **GPIO 25** | Red |
| **G1** | Green Upper | **GPIO 26** | Green |
| **B1** | Blue Upper | **GPIO 27** | Blue |
| **R2** | Red Lower | **GPIO 14** | Red/White |
| **G2** | Green Lower | **GPIO 12** | Green/White |
| **B2** | Blue Lower | **GPIO 13** | Blue/White |
| **A** | Row Select A | **GPIO 23** | Yellow |
| **B** | Row Select B | **GPIO 19** | Orange |
| **C** | Row Select C | **GPIO 5** | Brown |
| **D** | Row Select D | **GPIO 17** | Purple |
| **E** | Row Select E | **GPIO 18** | Gray |
| **CLK** | Clock | **GPIO 16** | White |
| **LAT** | Latch | **GPIO 4** | Yellow/Green |
| **OE** | Output Enable | **GPIO 15** | Black |
| **GND** | Ground | **GND** | Black |

## HUB75 Connector Layout

```
┌─────────────────────┐
│  ┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐  │  Top Row
│  └─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘  │
│   1  2  3  4  5  6  7  8   │
│                             │
│  ┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐┌─┐  │  Bottom Row
│  └─┘└─┘└─┘└─┘└─┘└─┘└─┘└─┘  │
│   9 10 11 12 13 14 15 16   │
└─────────────────────┐
                      │
                    Locking Tab
```

### Pin Numbers

**Top Row (left to right):**
1. R1
2. G1
3. B1
4. (blank)
5. R2
6. G2
7. B2
8. E

**Bottom Row (left to right):**
9. A
10. B
11. C
12. D
13. CLK
14. LAT
15. OE
16. GND

## Power Connections

**LED Matrix Power:**
```
5V Supply (+) ──────┬───> Matrix 5V IN
                    │
ESP32 GND ──────────┴───> Matrix GND
         └──────────────> 5V Supply (-)
```

**Important:**
- **Common ground** required between ESP32 and power supply
- **Never** power matrix from ESP32's 5V pin
- Use separate 5V supply rated 4A+ for the matrix

## Verification Checklist

Before powering on:

- [ ] All RGB data pins (R1, G1, B1, R2, G2, B2) connected
- [ ] All row select pins (A, B, C, D, E) connected
- [ ] Clock, Latch, Output Enable connected
- [ ] Ground connections secure
- [ ] 5V power supply connected to matrix only
- [ ] Common ground between ESP32 and power supply
- [ ] No shorts between adjacent pins

## Firmware Pin Configuration

If you need to change pins, edit these defines in the firmware:

```cpp
// In firmware/src/main.cpp, modify initMatrix():

HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,  // 64
    PANEL_RES_Y,  // 64
    PANEL_CHAIN   // 1
);

// Default pins from library:
// R1: 25, G1: 26, B1: 27
// R2: 14, G2: 12, B2: 13
// A: 23, B: 19, C: 5, D: 17
// E: 18, CLK: 16, LAT: 4, OE: 15

// To override:
mxconfig.gpio.r1 = 25;
mxconfig.gpio.g1 = 26;
// ... etc
```

## Troubleshooting Wiring

| Symptom | Check These Pins |
|---------|-----------------|
| Completely blank | CLK, LAT, OE, Power |
| Wrong colors | R1, G1, B1, R2, G2, B2 |
| Horizontal stripes | A, B, C, D, E |
| Flickering | CLK, OE, Ground |
| Top/bottom half wrong | Check R2/G2/B2 |

## Testing Individual Pins

To test if pins are working, you can add debug code:

```cpp
void testPin(int pin) {
    pinMode(pin, OUTPUT);
    for (int i = 0; i < 10; i++) {
        digitalWrite(pin, HIGH);
        delay(100);
        digitalWrite(pin, LOW);
        delay(100);
    }
}
```

## Panel Types

This pinout is for **64x64 panels**. If you have a different size:

| Panel Size | E Pin Required? | Notes |
|-----------|-----------------|-------|
| 32x32 | No | Don't connect E |
| 64x32 | No | Don't connect E |
| 64x64 | **Yes** | Connect E to GPIO 18 |

---

**Pro Tip:** Use colored dupont wires matching the signal type (red for R1/R2, green for G1/G2, etc.) to make troubleshooting easier!
