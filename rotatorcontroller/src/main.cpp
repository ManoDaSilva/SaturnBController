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
int pulseWidthMicros = 300;  // microseconds
int millisBtwnSteps = 3000;

//Functions declarations
void moveAxis(int axis, int deg, bool dir);
void initHw();
void homeSystem();
void homeAxis(int axis);

void recvWithStartEndMarkers();
void showNewData();



//Internal variables
const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;



void setup() {
  initHw();
  Serial.println(F("Rotator is alive!"));
}

void loop() {
  //TODO: replace this by serial handling.
  Serial.println(F("Running clockwise"));
  moveAxis(1,10,1);
  delay(1000); // One second delay
  Serial.println(F("Running counter-clockwise"));
  moveAxis(1,10,0);
  delay(1000);
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
  int steps = deg*PULLEYRATE*STEPSPERREV/360;
  for (int i = 0; i < steps; i++) {
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
  homeAxis(1);
  homeAxis(2);
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
  moveAxis(axis,axismax,1);
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







void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChars);
        newData = false;
    }
}