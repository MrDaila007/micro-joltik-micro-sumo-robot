# Micro Joltik Pinout

## Motor Driver (H-Bridge)

| Pin | Function     | Description          |
|-----|-------------|----------------------|
| 1   | PWMB        | Left motor PWM speed |
| 2   | BIN_1       | Left motor direction |
| 3   | BIN_2       | Left motor direction |
| 4   | STBY        | Standby (HIGH=active)|
| 5   | AIN_2       | Right motor direction|
| 6   | AIN_1       | Right motor direction|
| 7   | PWMA        | Right motor PWM speed|

## Sensors

| Pin | Function     | Description                     |
|-----|-------------|---------------------------------|
| A0  | EDGE_M      | Analog IR edge/line sensor      |
| 10  | OPPONENT_FL | Front-left IR proximity (38kHz) |
| 11  | OPPONENT_FR | Front-right IR proximity (38kHz)|
| 12  | OPPONENT_FC | Front-center IR proximity (38kHz)|

## Peripherals

| Pin | Function | Description              |
|-----|---------|--------------------------|
| 13  | SenGND  | Sensor ground control    |
| 14  | IRVCC   | IR sensor power control  |
| 15  | IR_PIN  | RC5 IR remote receiver   |
| 16  | PIN     | NeoPixel RGB LED (1 pixel)|
