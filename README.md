# Micro Joltik — Micro Sumo Robot

Micro Joltik is an autonomous micro sumo robot that detects opponents using IR proximity sensors and pushes them out of the arena. It features edge detection to stay within the ring boundary and RC5 remote control for start/stop.

## Hardware

- **MCU:** RP2040-zero
- **Motor driver:** TB6612FNG H-bridge driver
- **Motors:** 2× micro gear motors (tank drive)
- **Opponent detection:** 3× Pololu 38kHz IR proximity sensors (front left, center, right)
- **Edge detection:** 1× analog IR line sensor
- **Feedback:** 1× NeoPixel RGB LED
- **Control:** RC5 IR remote receiver
- **Chassis:** Plastic base with SolidWorks-designed 3D-printed parts

## Repository Structure

```
FIRMWARE/
  MicroJoltik_test_Firmware/   # Original test firmware
  MicroJoltik_stable/          # Stable firmware with bug fixes
CAD/                           # SolidWorks mechanical design files
doc/                           # Documentation (changelog, pinout)
```

## Building & Flashing

**Requirements:**
- Arduino IDE (or PlatformIO / VS Code with Arduino extension)
- Libraries: `Adafruit_NeoPixel`, `RC5` (install via Library Manager)

**Steps:**
1. Open `FIRMWARE/MicroJoltik_stable/MicroJoltik_stable.ino` in Arduino IDE
2. Select board: RP2040-Zero
3. Verify (Ctrl+R) to compile
4. Upload (Ctrl+U) to flash

## Configuration

Key `#define` options at the top of the firmware:

| Define            | Description                                |
|-------------------|--------------------------------------------|
| `Back_Start`      | Enable reverse-and-180° startup routine    |
| `ENABLE_LINE_SEN` | Enable edge detection sensor               |
| `SWAP_MOTORS`     | Swap left/right motor wiring               |
| `DEBUG_ENABLE`    | Enable serial debug output (9600 baud)     |

Speed constants (`MAX_SPEED`, `ATTACK_SPEED`, `SEARCH_SPEED_1/2`, etc.) can be tuned at the top of the file.

## Robot Behavior

1. **Startup** — Waits for RC5 remote start command (LED shows magenta). In debug mode, prints sensor states to serial for calibration.
2. **Start routine** — Backs up, turns a random direction 180°, charges forward, then scans for opponent.
3. **Search** — Spins in a circular pattern looking for opponent with IR sensors.
4. **Attack** — Charges at detected opponent. All three sensors = MAX_SPEED. Single sensor = turn toward opponent.
5. **Edge backoff** — Reverses and U-turns when the line sensor detects the arena boundary.
6. **Stop** — RC5 remote stop command halts all motors (LED shows red).

## License

Apache License 2.0
