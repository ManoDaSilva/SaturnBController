


// PIN DEFINITIONS
#define ENPIN 8
#define STEPAZPIN  2 //X.STEP
#define DIRAZPIN  5 // X.DIR
#define STEPELPIN 3 //Y.STEP ==> reassigned Y axis to Z, probably a hardware failure.
#define DIRELPIN 6 // Y.DIR
//#define STEPZPIN 4 //Z.STEP
//#define DIRZPIN 7 // Z.DIR


//MECHANICAL DEFINITIONS
#define PULLEYRATE 30 //calculated 24 
#define STEPSPERREV 200

#define STOPOFFSETAZ 10 //offset from endstop, in degrees
#define STOPOFFSETEL 24 //offset from endstop, in degrees

#define MAXDEGAZ 540
#define MAXDEGEL 140

#define PWMMICROS 400  // microseconds
#define STEPMILLIS 4000


