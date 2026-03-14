// Wiring Connections.
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#define PIN 16       // номер порта к которому подключен модуль
#define NUMPIXELS 1  // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


#include <RC5.h>
int IR_PIN = 15;
unsigned long t0;
RC5 rc5(IR_PIN);
unsigned char dohio;

#define EDGE_M A0       // Right edge sensor
#define OPPONENT_FR 11  // Front right opponent sensor
#define OPPONENT_FC 12  // Front center opponent sensor
#define OPPONENT_FL 10  // Front left opponent sensor

#define SenGND 13
#define IRVCC 14

#define Back_Start
#define ENABLE_LINE_SEN


//#define DEBUG_ENABLE

bool result = false;
int sensorValue = 0;

////////////////////////////////////
//////////// SPEED ////////////////
//////////////////////////////////
#define MAX_SPEED 255
#define STRAIGHT_SPEED 150
#define SEARCH_SPEED_1 100
#define SEARCH_SPEED_2 100
#define ROTATE_TANK_SPEED 150
#define ROTATE_SPEED 150
#define BRAKEOUT_SPEED 150
#define ATACK_SPEED 200

////////////////////////////////
int difference = 0;
uint32_t myTimer1;
uint8_t attackZone = 0;  //0-12


int PWMA = 7;
int AIN_2 = 5;
int AIN_1 = 6;
int stdby = 4;
int BIN_1 = 2;
int BIN_2 = 3;
int PWMB = 1;

//#define SVAP_MOTORS

// Direction.
#define LEFT 0
#define RIGHT 1

// Global variables.
uint8_t searchDir = LEFT;


// Configure the motor driver.


/*******************************************************************************
   Start Routine
   This function should be called once only when the game start.
 *******************************************************************************/
void startRoutine() {
  // Start delay.
  uint8_t Rand_time = random(150,240);
#ifdef Back_Start

  drive(-BRAKEOUT_SPEED+5, -BRAKEOUT_SPEED); // Go Back.
  delay(50);
  
  bool rand_DIR = random(0,2);
  if(rand_DIR)
  {
    searchDir = RIGHT;
    drive(ROTATE_TANK_SPEED, -ROTATE_TANK_SPEED); // Turn round
    delay(210);
  }
  else
  {
    searchDir = LEFT;
    drive(-ROTATE_TANK_SPEED, ROTATE_TANK_SPEED); // Turn round
    delay(210);
  }

  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);
  delay(210);

#else

  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);
  delay(400);

#endif
  // Go straight.


  // Turn left until opponent is detected.
    if(searchDir = RIGHT)
  {
    searchDir = RIGHT;
    drive(ROTATE_TANK_SPEED, -ROTATE_TANK_SPEED); // Turn round
    delay(210);
  }
  else
  {
    searchDir = LEFT;
    drive(-ROTATE_TANK_SPEED, ROTATE_TANK_SPEED); // Turn round
    delay(210);
  }
  uint32_t startTimestamp = millis();
  while (!digitalRead(OPPONENT_FC)) {
    // Quit if opponent is not found after timeout.
    if (millis() - startTimestamp > 300) {
      break;
    }
  }
}


/*******************************************************************************
   Back Off
   This function should be called when the ring edge is detected.
 *******************************************************************************/
void backoff(uint8_t dir) {
  // Stop the motors.

  // Reverse.
  drive(-BRAKEOUT_SPEED, -BRAKEOUT_SPEED);
#ifdef DEBUG_ENABLE
  Serial.println("Reverse");
#endif

  delay(100);

  // Stop the motors.
  drive(0, 0);

  delay(100);

  // Rotate..
  if (dir == LEFT) {
    drive(-ROTATE_SPEED, ROTATE_SPEED);

#ifdef DEBUG_ENABLE
    Serial.println("Dir_Left");
#endif


  } else {
    drive(ROTATE_SPEED, -ROTATE_SPEED);

#ifdef DEBUG_ENABLE
    Serial.println("Dir_Right");
#endif
  }
  delay(100);

  // Start looking for opponent.
  // Timeout after a short period.
  uint32_t uTurnTimestamp = millis();
  while (millis() - uTurnTimestamp < 200) {
    // Opponent is detected if either one of the opponent sensor is triggered.
    if (!digitalRead(OPPONENT_FC) || !digitalRead(OPPONENT_FR) || !digitalRead(OPPONENT_FL)) {

      // Stop the motors.
      drive(0, 0);
      delay(50);
        #ifdef DEBUG_ENABLE
          Serial.println("BrakeOut Trig");
        #endif

      // Return to the main loop and run the attach program.
      return;
    }
  }

  // If opponent is not found, move forward and continue searching in the main loop..

  drive(STRAIGHT_SPEED, STRAIGHT_SPEED);

  delay(100);
}


/*******************************************************************************
   Search
 *******************************************************************************/
void search() {
  // Move in circular motion.
  if (searchDir == LEFT) 
  {
    drive(-SEARCH_SPEED_1, SEARCH_SPEED_2);
      #ifdef DEBUG_ENABLE
        Serial.println("Search_Right");
      #endif
  } 
  else if (searchDir == RIGHT) 
  {
    drive(SEARCH_SPEED_2, -SEARCH_SPEED_1);
      #ifdef DEBUG_ENABLE
        Serial.println("Search_Right");
      #endif
  }
}


/*******************************************************************************
   Attack
   Track and attack the opponent in full speed.
   Do nothing if opponent is not found.
 *******************************************************************************/
void attack() {
  uint32_t attackTimestamp = millis();

  // Opponent in front center.
  // Go straight in full speed.
  if (!digitalRead(OPPONENT_FC)) 
  {
    drive(ATACK_SPEED, ATACK_SPEED);
    #ifdef DEBUG_ENABLE
      Serial.println("Attack Front");
    #endif
  }

  else if (!digitalRead(OPPONENT_FC) && !digitalRead(OPPONENT_FR) && !digitalRead(OPPONENT_FL)) 
  {
    drive(MAX_SPEED, MAX_SPEED);
      #ifdef DEBUG_ENABLE
        Serial.println("Attack Front Max speed");
      #endif
  }

  // Opponent in front left.
  // Turn left.
  else if (!digitalRead(OPPONENT_FL)) {
    searchDir == LEFT;
    drive(0, ROTATE_TANK_SPEED);
      #ifdef DEBUG_ENABLE
        Serial.println("Attack Front left");
      #endif
  }

  // Opponent in front right.
  // Turn right.
  else if (!digitalRead(OPPONENT_FR)) {
        searchDir == RIGHT;
    drive(ROTATE_TANK_SPEED, 0);
      #ifdef DEBUG_ENABLE
        Serial.println("Attack Front right");
      #endif
  }

 
/*
  // Opponent in left side.
  // Rotate left until opponent is in front.
  else if (!digitalRead(OPPONENT_FL)) {
    searchDir == LEFT;
    drive(-ROTATE_SPEED, ROTATE_SPEED);
#ifdef DEBUG_ENABLE
    Serial.println("Attack LEFT");
#endif
    while (!digitalRead(OPPONENT_FC)) {
      // Quit if opponent is not found after timeout.
      if (millis() - attackTimestamp > 400) {
        break;
      }
    }
  }

  // Opponent in right side.
  // Rotate right until opponent is in front.
  else if (!digitalRead(OPPONENT_FR)) {
    searchDir == RIGHT;
    drive(ROTATE_SPEED, -ROTATE_SPEED);
      #ifdef DEBUG_ENABLE
        Serial.println("Attack RIGHT");
      #endif
    while (!digitalRead(OPPONENT_FC)) {
      // Quit if opponent is not found after timeout.
      if (millis() - attackTimestamp > 400) {
        break;
      }
    }
  }
  */
}




/*******************************************************************************
   Setup
   This function runs once after reset.
 *******************************************************************************/
void setup() {

EEPROM. begin(512);
unsigned char value;
int address = 100;
EEPROM.get(address, value);
dohio = value;


#ifdef DEBUG_ENABLE
  Serial.begin(9600);
#endif

#ifdef DEBUG_ENABLE
  Serial.println("Robot has been started!");
  Serial.print("Dohio = ");
  Serial.println(dohio);
#endif


  pinMode(BIN_1, OUTPUT);
  pinMode(BIN_2, OUTPUT);
  pinMode(AIN_1, OUTPUT);
  pinMode(AIN_2, OUTPUT);

  pinMode(stdby, OUTPUT);
  pinMode(SenGND, OUTPUT);
  pinMode(IRVCC, OUTPUT);

  pinMode(OPPONENT_FL, INPUT);
  pinMode(OPPONENT_FC, INPUT);
  pinMode(OPPONENT_FR, INPUT);
  digitalWrite(stdby, 1);
  digitalWrite(SenGND, 0);
  digitalWrite(IRVCC, 1);

  pixels.begin();
  pixels.clear();  // Set all pixel colors to 'off'
  Blink(255, 255, 0);
  // Stop the motors.

  drive(0, 0);

  pixels.setPixelColor(0, pixels.Color(125, 0, 125));
  pixels.show();

  while (chekIReceive()) {
    // While waiting, show the status of the edge sensor for easy calibration.
    

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

  // Wait until button is released.
  while (!chekIReceive())
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

  // Turn on the LEDs.

  startRoutine();
}

/*******************************************************************************
   Main program loop.
 *******************************************************************************/
void loop() {

  #ifdef ENABLE_LINE_SEN
  
  // Edge is detected on the left.
  if (!LineSens()) 
  {
    // Back off and make a U-Turn to the right.
    backoff(RIGHT);

    // Toggle the search direction.
    searchDir ^= 1;
  }

  // Edge is not detected.
  else {
    
  #endif
  // Keep searching if opponent is not detected.
  if (digitalRead(OPPONENT_FC) && digitalRead(OPPONENT_FL) && digitalRead(OPPONENT_FR)) {
    #ifdef DEBUG_ENABLE
    Serial.println("GO SEARCH void");
    #endif
    search();
  }

  // Attack if opponent is in view.
  else 
  {
    #ifdef DEBUG_ENABLE
    Serial.println("GO to Attack void");
    #endif
  attack();

  }


  // Stop the robot if the button is pressed.
  if (chekIReceive() == 0) {
    while (true) {
      pixels.setPixelColor(0, pixels.Color(150, 0, 0));
      pixels.show();  
      drive(0, 0);
    }
  }
}
#ifdef ENABLE_LINE_SEN
}
#endif

void drive(int leftS, int rightS) {

#ifdef SVAP_MOTORS
  int left = rightS;
  int right = leftS;
#else
  int left = leftS;
  int right = rightS;
#endif

  left = constrain(left, -255, 255);    // жестко ограничиваем диапозон значений
  right = constrain(right, -255, 255);  // жестко ограничиваем диапозон значений


  if (left >= 0) {
    digitalWrite(BIN_1, LOW);
    digitalWrite(BIN_2, HIGH);
  } else {
    digitalWrite(BIN_1, HIGH);
    digitalWrite(BIN_2, LOW);
  }

  if (right >= 0) {
    digitalWrite(AIN_1, LOW);
    digitalWrite(AIN_2, HIGH);
  } else {
    digitalWrite(AIN_1, HIGH);
    digitalWrite(AIN_2, LOW);
  }


  analogWrite(PWMA, abs(right));
  analogWrite(PWMB, abs(left));
}


void Blink(int R, int G, int B) {
  pixels.clear();  // Set all pixel colors to 'off'
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

bool chekIReceive() {
  unsigned char toggle;
  unsigned char address;
  unsigned char command;
  if (rc5.read(&toggle, &address, &command)) {
    if (address == 11) {
      if(dohio != command)
      {
        int address = 100;
        EEPROM.put(address, command);
        EEPROM.commit();
        dohio = command;

        unsigned char value;
        int addressE = 100;
        EEPROM.get(addressE, value);
        if (value != dohio)
        {
          Blink(255, 0, 0);
          #ifdef DEBUG_ENABLE
            Serial.println("EEPROM write ERR");
          #endif
        }

        
      }
      dohio = command;
      Blink(0, 0, 255);
    }
    if (address == 7 && command == dohio) {
      result = false;
      pixels.setPixelColor(0, pixels.Color(150, 0, 0));
      pixels.show();  
    }
    if (address == 7 && command == dohio + 1) {
      result = true;
      pixels.setPixelColor(0, pixels.Color(0, 150, 0));
      pixels.show();  
    }
  }
  return result;
}


bool LineSens() {
  bool State = 0;
  sensorValue = analogRead(EDGE_M);
  if (sensorValue < 200) {
    State = 0;
  } else if (sensorValue > 500) {
    State = 1;
  }
  return State;
}
