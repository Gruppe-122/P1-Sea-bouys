#include "src/accel.h"
#include "src/amp.h"
#include "src/gps.h"
#include "include\RadioLib-master\src\RadioLib.h"

void setup() {
  Serial.begin(115200);

  String hello = helloWorld();
  Serial.println(hello);
}

void loop() {
  // put your main code here, to run repeatedly:
}
