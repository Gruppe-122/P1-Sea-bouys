#ifndef ACCEL_H
#define ACCEL_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>

int accelSetup();
int calibrate();
int accelerometer();

#endif
