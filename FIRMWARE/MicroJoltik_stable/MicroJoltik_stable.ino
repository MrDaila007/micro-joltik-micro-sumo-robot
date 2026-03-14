// ============================================================================
//  Micro Joltik — Micro Sumo Robot Firmware (Stable)
//  MCU: RP2040-Zero
//  Wiring connections and pin definitions below.
// ============================================================================

// --- NeoPixel RGB LED ---
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#define PIN 16       // GPIO pin connected to the NeoPixel data line
#define NUMPIXELS 1  // Single RGB LED on the board
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// --- RC5 IR Remote Receiver ---
#include <RC5.h>
int IR_PIN = 15;        // GPIO pin for the IR receiver module
unsigned long t0;
RC5 rc5(IR_PIN);
unsigned char dohio;     // Stores the current dohyo (arena) channel from EEPROM

// --- Sensor Pin Definitions ---
// IR sensors read LOW when an object is detected, HIGH when clear.
#define EDGE_M A0       // Analog edge/line sensor (detects white arena boundary)
#define OPPONENT_FR 11  // Front-right IR proximity sensor (38kHz Pololu)
#define OPPONENT_FC 12  // Front-center IR proximity sensor (38kHz Pololu)
#define OPPONENT_FL 10  // Front-left IR proximity sensor (38kHz Pololu)

// --- Sensor Power Pins ---
#define SenGND 13       // Sensor ground control pin (active LOW)
#define IRVCC 14        // IR sensor power control pin (active HIGH)

// --- Behavior Configuration ---
#define Back_Start       // Enable reverse-and-180° startup maneuver
#define ENABLE_LINE_SEN  // Enable edge detection (disable to ignore arena boundary)

//#define DEBUG_ENABLE   // Uncomment to enable serial debug output at 9600 baud

// --- Global State ---
bool result = false;     // Tracks IR remote start/stop state (true = running)
int sensorValue = 0;     // Last raw ADC reading from the edge sensor

// ============================================================================
//  Speed Constants (PWM values 0-255)
//  Tune these to match your motor/gear/wheel combination.
// ============================================================================
#define MAX_SPEED 255           // Absolute maximum motor output
#define STRAIGHT_SPEED 150      // Default forward driving speed
#define SEARCH_SPEED_1 100      // Inner wheel speed during search rotation
#define SEARCH_SPEED_2 100      // Outer wheel speed during search rotation
#define ROTATE_TANK_SPEED 150   // Both-wheels pivot turn speed
#define ROTATE_SPEED 150        // Single-direction rotation speed
#define BRAKEOUT_SPEED 150      // Reverse speed when escaping arena edge
#define ATTACK_SPEED 200        // Charge speed when opponent detected

// ============================================================================
//  Motor Driver Pin Definitions (H-Bridge: TB6612FNG or similar)
//  Each motor uses: 1 PWM pin (speed) + 2 direction pins (IN1/IN2)
// ============================================================================
int difference = 0;
uint32_t myTimer1;
uint8_t attackZone = 0;  // Attack zone tracking (0-12)

// Right motor (Motor A)
int PWMA = 7;    // PWM speed control for right motor
int AIN_2 = 5;   // Right motor direction pin 2
int AIN_1 = 6;   // Right motor direction pin 1

int stdby = 4;   // H-bridge standby pin (HIGH = motors enabled)

// Left motor (Motor B)
int BIN_1 = 2;   // Left motor direction pin 1
int BIN_2 = 3;   // Left motor direction pin 2
int PWMB = 1;    // PWM speed control for left motor

//#define SWAP_MOTORS  // Uncomment if left/right motors are wired in reverse

// --- Direction Constants ---
#define LEFT 0
#define RIGHT 1

// Current search/spin direction. Updated during attack to remember
// which side the opponent was last seen, so search resumes that way.
uint8_t searchDir = LEFT;


// ============================================================================
//  Start Routine
//  Called once when the match begins (after RC5 remote start signal).
//  Executes an opening maneuver to find the opponent quickly.
// ============================================================================
void startRoutine() {
  uint8_t Rand_time = random(150, 240);  // Random delay seed (unused currently)

#ifdef Back_Start
  // --- Back-start strategy: reverse, spin 180°, then charge forward ---

  // Step 1: Drive backward briefly to create distance from starting position
  drive(-BRAKEOUT_SPEED + 5, -BRAKEOUT_SPEED);
  delay(50);

  // Step 2: Pick a random direction and spin 180° to face the arena
  bool rand_DIR = random(0, 2);  // 50/50 chance: 0 = LEFT, 1 = RIGHT
  if (rand_DIR)
  {
    searchDir = RIGHT;
    drive(ROTATE_TANK_SPEED, -ROTATE_TANK_SPEED);  // Pivot right (tank turn)
    delay(210);
  }
  else
  {
    searchDir = LEFT;
    drive(-ROTATE_TANK_SPEED, ROTATE_TANK_SPEED);  // Pivot left (tank turn)
    delay(210);
  }

  // Step 3: Charge forward toward the center of the arena
  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);
  delay(210);

#else
  // --- Simple start strategy: just charge straight forward ---
  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);
  delay(400);

#endif

  // Step 4: Rotate in the chosen direction to scan for opponent
  if (searchDir == RIGHT)
  {
    drive(ROTATE_TANK_SPEED, -ROTATE_TANK_SPEED);  // Pivot right
    delay(210);
  }
  else
  {
    drive(-ROTATE_TANK_SPEED, ROTATE_TANK_SPEED);  // Pivot left
    delay(210);
  }

  // Step 5: Wait for the front-center sensor to detect an opponent (300ms timeout)
  // LOW = opponent detected, HIGH = no opponent
  uint32_t startTimestamp = millis();
  while (!digitalRead(OPPONENT_FC)) {
    if (millis() - startTimestamp > 300) {
      break;  // Give up scanning, continue to main loop
    }
  }
}


// ============================================================================
//  Back Off
//  Called when the edge sensor detects the white arena boundary.
//  Reverses away from the edge, rotates, and tries to reacquire the opponent.
//  @param dir - direction to rotate after reversing (LEFT or RIGHT)
// ============================================================================
void backoff(uint8_t dir) {

  // Step 1: Reverse away from the arena edge at full breakout speed
  drive(-BRAKEOUT_SPEED, -BRAKEOUT_SPEED);
#ifdef DEBUG_ENABLE
  Serial.println("Reverse");
#endif
  delay(100);

  // Step 2: Brief stop before rotating (helps prevent wheel slip)
  drive(0, 0);
  delay(100);

  // Step 3: Rotate away from the edge in the specified direction
  if (dir == LEFT) {
    drive(-ROTATE_SPEED, ROTATE_SPEED);  // Pivot left
#ifdef DEBUG_ENABLE
    Serial.println("Dir_Left");
#endif
  } else {
    drive(ROTATE_SPEED, -ROTATE_SPEED);  // Pivot right
#ifdef DEBUG_ENABLE
    Serial.println("Dir_Right");
#endif
  }
  delay(100);

  // Step 4: While rotating, check if any sensor picks up the opponent (200ms window)
  uint32_t uTurnTimestamp = millis();
  while (millis() - uTurnTimestamp < 200) {
    if (!digitalRead(OPPONENT_FC) || !digitalRead(OPPONENT_FR) || !digitalRead(OPPONENT_FL)) {
      // Opponent found during U-turn — stop and return to main loop for attack
      drive(0, 0);
      delay(50);
#ifdef DEBUG_ENABLE
      Serial.println("BrakeOut Trig");
#endif
      return;
    }
  }

  // Step 5: No opponent found — drive forward and let the main loop handle searching
  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);
  delay(100);
}


// ============================================================================
//  Search
//  Spins the robot in a circular arc to scan for opponents.
//  Direction is determined by the global searchDir variable, which is
//  updated during attack() when the opponent is last seen on one side.
// ============================================================================
void search() {
  // Differential drive creates a circular search path.
  // One wheel drives forward, the other reverses at search speed.
  if (searchDir == LEFT)
  {
    drive(-SEARCH_SPEED_1, SEARCH_SPEED_2);  // Spin left
#ifdef DEBUG_ENABLE
    Serial.println("Search_Left");
#endif
  }
  else if (searchDir == RIGHT)
  {
    drive(SEARCH_SPEED_2, -SEARCH_SPEED_1);  // Spin right
#ifdef DEBUG_ENABLE
    Serial.println("Search_Right");
#endif
  }
}


// ============================================================================
//  Attack
//  Tracks and charges at the opponent based on which sensors detect it.
//  Priority order (highest first):
//    1. All three sensors → MAX_SPEED (opponent dead center, full send)
//    2. Front center only → ATTACK_SPEED (opponent ahead, charge)
//    3. Front left only   → pivot left to align
//    4. Front right only  → pivot right to align
//  Also updates searchDir so if we lose the opponent, we search toward
//  the side it was last seen.
// ============================================================================
void attack() {
  uint32_t attackTimestamp = millis();

  // All three sensors detect opponent — it's right in front, go full power
  if (!digitalRead(OPPONENT_FC) && !digitalRead(OPPONENT_FR) && !digitalRead(OPPONENT_FL))
  {
    drive(MAX_SPEED, MAX_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front Max speed");
#endif
  }

  // Only front-center detects — opponent is ahead, charge at attack speed
  else if (!digitalRead(OPPONENT_FC))
  {
    drive(ATTACK_SPEED, ATTACK_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front");
#endif
  }

  // Only front-left detects — opponent is to the left, pivot to align
  else if (!digitalRead(OPPONENT_FL)) {
    searchDir = LEFT;   // Remember: opponent was last seen on the left
    drive(0, ROTATE_TANK_SPEED);  // Left wheel stopped, right wheel forward
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front left");
#endif
  }

  // Only front-right detects — opponent is to the right, pivot to align
  else if (!digitalRead(OPPONENT_FR)) {
    searchDir = RIGHT;  // Remember: opponent was last seen on the right
    drive(ROTATE_TANK_SPEED, 0);  // Left wheel forward, right wheel stopped
#ifdef DEBUG_ENABLE
    Serial.println("Attack Front right");
#endif
  }
}


// ============================================================================
//  Setup
//  Runs once after power-on or reset.
//  Initializes hardware, loads settings from EEPROM, and waits for
//  the RC5 remote start command before beginning the match.
// ============================================================================
void setup() {

  // Load the dohyo channel number from EEPROM (persists across power cycles).
  // This value is set via RC5 remote with address=11 and determines which
  // start/stop commands the robot responds to (for multi-robot arenas).
  EEPROM.begin(512);
  unsigned char value;
  int eepromAddr = 100;  // EEPROM storage address for the dohyo channel
  EEPROM.get(eepromAddr, value);
  dohio = value;

#ifdef DEBUG_ENABLE
  Serial.begin(9600);
  Serial.println("Robot has been started!");
  Serial.print("Dohio = ");
  Serial.println(dohio);
#endif

  // --- Configure motor driver pins as outputs ---
  pinMode(BIN_1, OUTPUT);
  pinMode(BIN_2, OUTPUT);
  pinMode(AIN_1, OUTPUT);
  pinMode(AIN_2, OUTPUT);

  // --- Configure power and control pins ---
  pinMode(stdby, OUTPUT);
  pinMode(SenGND, OUTPUT);
  pinMode(IRVCC, OUTPUT);

  // --- Configure sensor pins as inputs ---
  pinMode(OPPONENT_FL, INPUT);
  pinMode(OPPONENT_FC, INPUT);
  pinMode(OPPONENT_FR, INPUT);

  // --- Activate hardware ---
  digitalWrite(stdby, 1);   // Enable H-bridge (exit standby mode)
  digitalWrite(SenGND, 0);  // Set sensor ground LOW
  digitalWrite(IRVCC, 1);   // Power on IR proximity sensors

  // --- Initialize NeoPixel LED ---
  pixels.begin();
  pixels.clear();
  Blink(255, 255, 0);  // Yellow double-blink to indicate boot complete

  // Ensure motors are stopped
  drive(0, 0);

  // Show magenta LED while waiting for start command
  pixels.setPixelColor(0, pixels.Color(125, 0, 125));
  pixels.show();

  // --- Wait for RC5 remote start command ---
  // Robot sits idle displaying magenta LED. In debug mode, sensor states
  // are printed to serial for calibration and troubleshooting.
  while (checkIRReceive()) {
#ifdef DEBUG_ENABLE
    bool FRONT_L_State = digitalRead(OPPONENT_FL);
    bool FRONT_State = digitalRead(OPPONENT_FC);
    bool FRONT_R_State = digitalRead(OPPONENT_FR);
    bool LINE_State = LineSens();

    // Output format: FL FC FR EDGE ADC_VALUE
    Serial.print(FRONT_L_State);
    Serial.print(" ");
    Serial.print(FRONT_State);
    Serial.print(" ");
    Serial.print(FRONT_R_State);
    Serial.print(" ");
    Serial.print(!LINE_State);
    Serial.println(" ");
    Serial.print(sensorValue);
    Serial.println(" ");
#endif
  }

  // Wait for the start button release (debounce)
  while (!checkIRReceive())
  {
    pixels.setPixelColor(0, pixels.Color(125, 0, 125));
    pixels.show();
#ifdef DEBUG_ENABLE
    bool FRONT_L_State = digitalRead(OPPONENT_FL);
    bool FRONT_State = digitalRead(OPPONENT_FC);
    bool FRONT_R_State = digitalRead(OPPONENT_FR);
    bool LINE_State = LineSens();

    Serial.print(FRONT_L_State);
    Serial.print(" ");
    Serial.print(FRONT_State);
    Serial.print(" ");
    Serial.print(FRONT_R_State);
    Serial.print(" ");
    Serial.print(!LINE_State);
    Serial.println(" ");
    Serial.print(sensorValue);
    Serial.println(" ");
#endif
  }

  // Match begins — run the opening maneuver
  startRoutine();
}

// ============================================================================
//  Main Loop
//  Runs continuously after setup. Implements the core behavior state machine:
//    1. Check edge sensor → backoff if arena boundary detected
//    2. Check opponent sensors → attack if detected, search if not
//    3. Check RC5 remote → stop if stop command received
// ============================================================================
void loop() {

#ifdef ENABLE_LINE_SEN
  // --- Edge Detection ---
  // LineSens() returns true (1) if on the arena surface, false (0) if edge detected.
  // When edge is detected, reverse and toggle search direction.
  if (!LineSens())
  {
    backoff(RIGHT);
    searchDir ^= 1;  // Flip search direction (LEFT <-> RIGHT) using XOR
  }
  else
  {
#endif

  // --- Opponent Detection ---
  // All IR sensors read HIGH when no object is present.
  // If all three sensors are clear, keep searching.
  if (digitalRead(OPPONENT_FC) && digitalRead(OPPONENT_FL) && digitalRead(OPPONENT_FR)) {
#ifdef DEBUG_ENABLE
    Serial.println("GO SEARCH void");
#endif
    search();
  }
  // At least one sensor detects the opponent — switch to attack mode.
  else
  {
#ifdef DEBUG_ENABLE
    Serial.println("GO to Attack void");
#endif
    attack();
  }

#ifdef ENABLE_LINE_SEN
  }  // Close the else block from edge detection
#endif

  // --- Remote Stop Check ---
  // If the RC5 remote sends a stop command, halt the robot permanently.
  // The red LED indicates stopped state. Requires hardware reset to restart.
  if (checkIRReceive() == 0) {
    while (true) {
      pixels.setPixelColor(0, pixels.Color(150, 0, 0));
      pixels.show();
      drive(0, 0);
    }
  }
}


// ============================================================================
//  Drive
//  Controls both motors simultaneously. Positive values = forward, negative = reverse.
//  Handles direction pin logic for the H-bridge and applies PWM speed.
//  @param leftS  - left motor speed (-255 to 255)
//  @param rightS - right motor speed (-255 to 255)
// ============================================================================
void drive(int leftS, int rightS) {

#ifdef SWAP_MOTORS
  // If motors are wired in reverse, swap the values
  int left = rightS;
  int right = leftS;
#else
  int left = leftS;
  int right = rightS;
#endif

  // Clamp speed values to valid PWM range
  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  // Set left motor (B) direction via H-bridge pins
  if (left >= 0) {
    digitalWrite(BIN_1, LOW);
    digitalWrite(BIN_2, HIGH);   // Forward
  } else {
    digitalWrite(BIN_1, HIGH);
    digitalWrite(BIN_2, LOW);    // Reverse
  }

  // Set right motor (A) direction via H-bridge pins
  if (right >= 0) {
    digitalWrite(AIN_1, LOW);
    digitalWrite(AIN_2, HIGH);   // Forward
  } else {
    digitalWrite(AIN_1, HIGH);
    digitalWrite(AIN_2, LOW);    // Reverse
  }

  // Apply speed via PWM (absolute value since direction is set above)
  analogWrite(PWMA, abs(right));
  analogWrite(PWMB, abs(left));
}


// ============================================================================
//  Blink
//  Flashes the NeoPixel LED twice in the specified color.
//  Used for visual status feedback (boot, error, mode change).
//  @param R, G, B - color values (0-255)
// ============================================================================
void Blink(int R, int G, int B) {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(R, G, B));
  pixels.show();
  delay(500);
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
  delay(500);
  pixels.setPixelColor(0, pixels.Color(R, G, B));
  pixels.show();
  delay(500);
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
}

// ============================================================================
//  checkIRReceive
//  Reads RC5 IR remote commands and handles two functions:
//
//  1. Channel setting (address=11): Stores the dohyo channel in EEPROM.
//     This lets multiple robots share an arena with different remotes.
//     Blue blink confirms the channel was set.
//
//  2. Start/Stop (address=7):
//     - command == dohio     → STOP  (red LED, result = false)
//     - command == dohio + 1 → START (green LED, result = true)
//
//  @return true if robot should be running, false if stopped
// ============================================================================
bool checkIRReceive() {
  unsigned char toggle;
  unsigned char address;
  unsigned char command;

  if (rc5.read(&toggle, &address, &command)) {

    // --- Channel configuration command (address 11) ---
    if (address == 11) {
      if (dohio != command)
      {
        // Save new channel to EEPROM
        int eepromAddr = 100;
        EEPROM.put(eepromAddr, command);
        EEPROM.commit();
        dohio = command;

        // Verify the write was successful
        unsigned char value;
        EEPROM.get(eepromAddr, value);
        if (value != dohio)
        {
          Blink(255, 0, 0);  // Red blink = EEPROM write error
#ifdef DEBUG_ENABLE
          Serial.println("EEPROM write ERR");
#endif
        }
      }
      dohio = command;
      Blink(0, 0, 255);  // Blue blink = channel set confirmation
    }

    // --- Stop command: address 7, command matches current channel ---
    if (address == 7 && command == dohio) {
      result = false;
      pixels.setPixelColor(0, pixels.Color(150, 0, 0));  // Red = stopped
      pixels.show();
    }

    // --- Start command: address 7, command = channel + 1 ---
    if (address == 7 && command == dohio + 1) {
      result = true;
      pixels.setPixelColor(0, pixels.Color(0, 150, 0));  // Green = running
      pixels.show();
    }
  }
  return result;
}


// ============================================================================
//  LineSens
//  Reads the analog edge sensor and returns whether the robot is safely
//  on the arena surface.
//
//  The sensor reads different ADC values based on surface reflectivity:
//    - < 200: Off the edge / white boundary line detected → return 0 (unsafe)
//    - > 500: On the dark arena surface → return 1 (safe)
//    - 200-500: Dead zone — returns previous state (hysteresis)
//
//  @return true (1) if on arena surface, false (0) if edge detected
// ============================================================================
bool LineSens() {
  bool State = 0;
  sensorValue = analogRead(EDGE_M);
  if (sensorValue < 200) {
    State = 0;  // Edge detected (white boundary)
  } else if (sensorValue > 500) {
    State = 1;  // Safe (dark arena surface)
  }
  // Values 200-500 are in the dead zone; State stays 0 (defaults to unsafe)
  return State;
}
