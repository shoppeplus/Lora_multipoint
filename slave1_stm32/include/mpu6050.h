#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

void mpuInit();
void readAccelXYZ(float &ax, float &ay, float &az);

#endif
