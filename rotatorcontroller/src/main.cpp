/*
Saturn B Az-El controller
Manoel - ON6RF


Uses a CNC Controller V3 board on an Arduino Uno. 


TODO: 
- Implement Hamlib
- Implement rate sensor sensing to avoid ramming for zeros

*/

#include <Arduino.h>
#include <stdlib.h>   // strtod
#include <string.h>   // strstr
#include "config.h"


//Functions declarations
void moveAxis(int axis, int deg, bool dir);
void initHw();
void homeSystem();
void homeAxis(int axis);

void handleCommand(const char* cmd);

float clampf(float v, float lo, float hi);
float norm360(float a);
bool gotoAzEl(float targetAz, float targetEl);
void handleEasycommLine(char* line);
bool parseEasycommAzEl(const char* line, float &az, float &el);
float chooseAzPhysicalTarget(float targetLogical);
float getAzLogical();


//Internal variables
const byte BUF_LEN = 32;
char receivedChars[BUF_LEN];

volatile bool isHomed = false;
// Physical position in degrees
float posAzPhys = 0.0f;
float posEl = 0.0f;


void setup() {
  initHw();
  Serial.println(F("Rotator is alive!"));
}

void loop() {
  static byte idx = 0;

  while (Serial.available() > 0) {
    char c = Serial.read();
    Serial.write(c);
    if (c == '\r') continue; // ignore CR

    if (c == '\n') {
      receivedChars[idx] = '\0';
      idx = 0;
      if (receivedChars[0] == '\0') return; // ignore empty line

      handleEasycommLine(receivedChars);
    }
    else if (idx < BUF_LEN - 1) {
      receivedChars[idx++] = c;
    }
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
    delayMicroseconds(PWMMICROS);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(STEPMILLIS);
  }
}

/*
Homes all axis, one after the other
No arguments
*/
void homeSystem(){
  /*Serial.println(F("Homing Azimuth..."));
  homeAxis(0);
  Serial.println(F("Moving Azimuth to 0 deg"));
  moveAxis(0,STOPOFFSETAZ,1);*/

  Serial.println(F("Homing Elevation..."));
  homeAxis(1);
  Serial.println(F("Moving Elevation to 0 deg"));
  moveAxis(1,STOPOFFSETEL,1);
  Serial.println(F("Homing Done!"));
  posAzPhys = 0.0f;
  posEl = 0.0f;
  isHomed = true;
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


/*
Clamps targets, computes deltas, rounds to integer degrees, updates posAz/posEl
Arguments: 
- targetAz: float, wished azimuth
- targetEl: float, wished elevation
*/
bool gotoAzEl(float targetAzLogical, float targetEl) {
  if (!isHomed) return false;

  // Elevation is straightforward
  targetEl = clampf(targetEl, 0.0f, (float)MAXDEGEL);

  // Azimuth: map logical [0,360) to best physical [0,MAXDEGAZ]
  float targetAzPhys = chooseAzPhysicalTarget(targetAzLogical);

  // Deltas in physical space
  float dAz = targetAzPhys - posAzPhys;
  float dEl = targetEl     - posEl;

  bool azDir = (dAz >= 0.0f);
  bool elDir = (dEl >= 0.0f);

  int azDeg = (int)lroundf(fabsf(dAz));
  int elDeg = (int)lroundf(fabsf(dEl));

  if (azDeg > 0) {
    moveAxis(0, azDeg, azDir);
    posAzPhys += azDir ? azDeg : -azDeg;
    posAzPhys = clampf(posAzPhys, 0.0f, (float)MAXDEGAZ);
  }

  if (elDeg > 0) {
    moveAxis(1, elDeg, elDir);
    posEl += elDir ? elDeg : -elDeg;
    posEl = clampf(posEl, 0.0f, (float)MAXDEGEL);
  }

  return true;
}

/*
Sanitizes Easycomm interface lines, with options to either home or return a position.
*/
void handleEasycommLine(char* line) {
  if (strcmp(line, "HOME") == 0) { 
    homeSystem(); 
    Serial.println(F("OK")); 
    return; }
  if (strcmp(line, "p") == 0 || strcmp(line, "P") == 0) {
    Serial.print("AZ"); 
    Serial.print(getAzLogical(), 1);
    Serial.print(" EL"); 
    Serial.println(posEl, 1);
    return;
  }

  float az, el;
  if (parseEasycommAzEl(line, az, el)) {
    if (gotoAzEl(az, el)) {
      Serial.print("AZ"); 
      Serial.print(getAzLogical(), 1);
      Serial.print(" EL"); 
      Serial.println(posEl, 1);
    } else {
      Serial.println(F("ERR NOTHOMED/LIMIT"));
    }
  } else {
    Serial.println(F("ERR PARSE"));
  }
}


/*
Logical Azimuth reporting
*/
float getAzLogical() {
  return norm360(posAzPhys);
}

/*Pick the best physical AZ target that matches a logical target in [0,360] 
Candidates: target + 360*k, for k in {0,1} that fit within [0, MAXDEGAZ]
*/
float chooseAzPhysicalTarget(float targetLogical) {
  targetLogical = norm360(targetLogical);

  float cand0 = targetLogical;        // k=0
  float best = cand0;
  float bestDist = fabsf(cand0 - posAzPhys);

  float cand1 = targetLogical + 360.0f;  // k=1
  if (cand1 <= (float)MAXDEGAZ) {
    float d1 = fabsf(cand1 - posAzPhys);
    if (d1 < bestDist) {
      bestDist = d1;
      best = cand1;
    }
  }

  // Also ensure we never command outside physical limits
  return clampf(best, 0.0f, (float)MAXDEGAZ);
}


/*Utility Helper to limit values*/
float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}


/*Normalize angle into [0,360)
*/
float norm360(float a) {
  a = fmodf(a, 360.0f);
  if (a < 0) a += 360.0f;
  return a;
}

/*
Parse Easycomm float commands
*/
bool parseEasycommAzEl(const char* line, float &az, float &el) {
  const char* pAZ = strstr(line, "AZ");
  if (!pAZ) return false;
  pAZ += 2; // skip "AZ"

  char* endPtr = nullptr;
  az = (float)strtod(pAZ, &endPtr);
  if (endPtr == pAZ) return false; // no number parsed

  const char* pEL = strstr(endPtr, "EL");
  if (!pEL) return false;
  pEL += 2; // skip "EL"

  el = (float)strtod(pEL, &endPtr);
  if (endPtr == pEL) return false;

  return true;
}