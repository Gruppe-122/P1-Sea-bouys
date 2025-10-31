#include "src/accel.h"

void setup() {
  Serial.begin(115200);

  String hello = helloWorld();
  Serial.println(hello);
}

void loop() {
  // put your main code here, to run repeatedly:
}
