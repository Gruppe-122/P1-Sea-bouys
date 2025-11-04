#include "src/accel.h"
#include "src/amp.h"
#include "src/gps.h"

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
}
