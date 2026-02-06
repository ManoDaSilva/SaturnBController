/*
Saturn B Az-El controller
By Manoel - ON6RF


Uses a CNC Controller V3 board on an Arduino Uno. 


TODO: 
- Implement Serial command parsing
- Implement Hamlib
- Implement rate sensor sensing to avoid ramming for zeros

*/

#include <Arduino.h>
#include "config.h"

//Config variables TODO: move to config.h
int pulseWidthMicros = 400;  // microseconds
int millisBtwnSteps = 4000;

//Functions declarations
void moveAxis(int axis, int deg, bool dir);
void initHw();
void homeSystem();
void homeAxis(int axis);

void handleCommand(const char* cmd);


//Internal variables
const byte BUF_LEN = 32;
char receivedChars[BUF_LEN];



void setup() {
  initHw();
  Serial.println(F("Rotator is alive!"));
  homeSystem();
  Serial.println(F("Homing Done!"));
  Serial.println(F("Enter command like: 045+ 090-"));
}

void loop() {
  //TODO: replace this by serial handling.
  static byte idx = 0;

  while (Serial.available() > 0) {
    char c = Serial.read();
    Serial.write(c);

    if (c == '\n') {
      receivedChars[idx] = '\0';
      idx = 0;
      handleCommand(receivedChars);
    }
    else if (idx < BUF_LEN - 1) {
      receivedChars[idx++] = c;
    }
    // overflow safely ignored
  }

}



/*
Moves the axis in the specified direction. 
Arguments: 
- int axis: which axis. 0 is for Az, 1 is for El
- int deg: movement in degrees (will be rounded up to the closest int degree for now)
- bool dir: direction
*/
void moveAxis(int axis, int deg, bool dir){
  int stepPin;
  int dirPin;
  switch (axis)
  {
  case 0:
    stepPin=STEPAZPIN;
    dirPin=DIRAZPIN;
    break;
  case 1:
    stepPin=STEPELPIN;
    dirPin=DIRELPIN;
    break;
  default:
    break;
  }
  digitalWrite(dirPin, dir);
  long steps = (long)deg * STEPSPERREV * PULLEYRATE / 360;
  for (long i = 0; i < steps; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(pulseWidthMicros);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(millisBtwnSteps);
  }
}

/*
Homes all axis, one after the other
No arguments
*/
void homeSystem(){
  //Serial.println(F("Homing Azimuth..."));
  //homeAxis(0);
  Serial.println(F("Homing Elevation..."));
  homeAxis(1);
  Serial.println(F("Moving Elevation to 0 deg"));
  moveAxis(1,STOPOFFSETEL,1);
}

/*
Homes one specific axis
Arguments:
- int axis: which axis. 0 is for Az, 1 is for El
TODO: Implement something cleaner than just ramming the axis against the endstops
*/
void homeAxis(int axis){
  int axismax = 0;
  switch (axis)
  {
  case 0:
    axismax=MAXDEGAZ;
    break;
  case 1:
    axismax=MAXDEGEL;
    break;
  default:
    break;
  }
  moveAxis(axis,axismax,0);
}


/*
Inititalizes the hardware (serial, pins, etc)
No arguments
*/
void initHw(){
  Serial.begin(9600);

  pinMode(ENPIN, OUTPUT);
  digitalWrite(ENPIN, LOW);

  pinMode(STEPAZPIN, OUTPUT);
  pinMode(STEPELPIN, OUTPUT);
  pinMode(DIRAZPIN, OUTPUT);
  pinMode(DIRELPIN, OUTPUT);
}


// --- Parse + sanitize + execute ---
void handleCommand(const char* cmd) {
  int azCmd, elCmd;
  bool azDir, elDir;
  char sign1 = 0, sign2 = 0, extra = 0;

  int n = sscanf(cmd, " %d%c %d%c %c", &azCmd, &sign1, &elCmd, &sign2, &extra);
  if (n != 4) {
    Serial.println(F("Parse error"));
    return;
  }

  if ((sign1 != '+') && (sign1 != '-')) return;
  if ((sign2 != '+') && (sign2 != '-')) return;

  azDir = (sign1 == '+');
  elDir = (sign2 == '+');

  // Safety limits
  if (azCmd < 0 || azCmd > 360) return;
  if (elCmd < 0 || elCmd > 360) return;

  moveAxis(0, azCmd, azDir);
  moveAxis(1, elCmd, elDir);
}