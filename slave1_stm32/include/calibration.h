#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>
#include "config.h"

// Axis mapping globals
extern int gravAxis;
extern int horzAxis1;
extern int horzAxis2;
extern const char* axisNames[];

// Tự phát hiện trục gravity và remap tọa độ
void calibrateOrientation();

#endif
