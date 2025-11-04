#ifndef ACCEL_H
#define ACCEL_H

#define tyngdeAcc 9.81  //m/s2
#define maxG 6

#include <Arduino.h>
#include <Wire.h>  //lader mig kommunikere på I2C
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

int accelSetup();
int calibrate();
int accelerometer();

#endif
