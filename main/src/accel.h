#ifndef ACCEL_H
#define ACCEL_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>

struct AccelData
{ // samler x y og z under en varibel = AccelData
    float x;
    float y;
    float z;
};

int accelSetup();
int calibrate();
int accelerometer();

#endif
