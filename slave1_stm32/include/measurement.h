#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <Arduino.h>
#include "config.h"

// Sample buffers
extern float samples[SAMPLE_SIZE];
#if ENABLE_MULTI_AXIS
extern float samplesX[SAMPLE_SIZE];
extern float samplesY[SAMPLE_SIZE];
#endif

// Smoothed output values (after EMA)
extern float topFreqsZ[NUM_TOP_FREQS];
extern float rmsZ, peakZ, crestFactorZ;
extern float topFreqX, rmsX, peakX;
extern float topFreqY, rmsY, peakY;

// State
extern int alertLevel;
extern unsigned long measureCount;
extern bool dataReady;
extern unsigned long lastMeasure;

void collectSamples();
void performMeasurement();
void resetEMA();
String buildResponse(unsigned long seqNum);

#endif
