#include "src/accel.h"
//#include "src/current.h"
#include "src/bouyGPS.h"

myGps myGps;

void setup() {
  Serial.begin(115200);
  accelSetup();
  calibrate();
}

void loop() {
  if (accelerometer() == 0){
    Serial.println("sending msg to port");
    delay(10000);
  };
  myGps.run();
}