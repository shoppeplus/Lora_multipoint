#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>
#include "config.h"

// Axis mapping (set by calibrateOrientation)
extern int gravAxis;       // Physical axis with gravity → logical Z
extern int horzAxis1;      // Horizontal axis 1 → logical X
extern int horzAxis2;      // Horizontal axis 2 → logical Y
extern const char* axisNames[];

// Detect gravity axis from 100 samples, remap logical axes
void calibrateOrientation();

#endif
