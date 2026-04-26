#include "calibration.h"
#include "mpu6050.h"

// Default axis mapping: physical Z = vertical
int gravAxis  = 2;
int horzAxis1 = 0;
int horzAxis2 = 1;
const char* axisNames[] = {"X", "Y", "Z"};

void calibrateOrientation() {
    float sum[3] = {0, 0, 0};

    for (int i = 0; i < CALIB_SAMPLES; i++) {
        float ax, ay, az;
        readAccelXYZ(ax, ay, az);
        sum[0] += ax;
        sum[1] += ay;
        sum[2] += az;
        delay(5);
    }

    float mean[3];
    float absMax = 0;
    int maxAxis = 2;

    for (int i = 0; i < 3; i++) {
        mean[i] = sum[i] / CALIB_SAMPLES;
        float absMean = fabsf(mean[i]);
        if (absMean > absMax) {
            absMax = absMean;
            maxAxis = i;
        }
    }

    gravAxis = maxAxis;

    // Assign remaining axes as horizontal
    int h = 0;
    int hAxes[2];
    for (int i = 0; i < 3; i++) {
        if (i != gravAxis) hAxes[h++] = i;
    }
    horzAxis1 = hAxes[0];
    horzAxis2 = hAxes[1];

    Serial.println("[CALIB] Raw means:");
    Serial.print("  X: "); Serial.print(mean[0], 4); Serial.println(" g");
    Serial.print("  Y: "); Serial.print(mean[1], 4); Serial.println(" g");
    Serial.print("  Z: "); Serial.print(mean[2], 4); Serial.println(" g");
    Serial.print("[CALIB] Gravity on ");
    Serial.print(axisNames[gravAxis]);
    Serial.print(" (mean="); Serial.print(mean[gravAxis], 4); Serial.println("g)");
    Serial.print("[CALIB] Map: V=");
    Serial.print(axisNames[gravAxis]);
    Serial.print(" H1="); Serial.print(axisNames[horzAxis1]);
    Serial.print(" H2="); Serial.println(axisNames[horzAxis2]);

    if (absMax < GRAVITY_THRESHOLD) {
        Serial.println("[CALIB] WARNING: No clear gravity! Using default Z");
        gravAxis = 2; horzAxis1 = 0; horzAxis2 = 1;
    }
}
