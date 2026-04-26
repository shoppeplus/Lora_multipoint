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

// Smoothed results (output)
extern float topFreqsZ[NUM_TOP_FREQS];
extern float rmsZ, peakZ, crestFactorZ;
extern float topFreqX, rmsX, peakX;
extern float topFreqY, rmsY, peakY;

// Alert
extern int alertLevel;
extern unsigned long measureCount;
extern bool dataReady;
extern unsigned long lastMeasure;

// Thu thập mẫu từ MPU6050 (có axis remap)
void collectSamples();

// FFT + EMA smoothing + alert detection
void performMeasurement();

// Reset EMA (sau recalibrate)
void resetEMA();

// Build LoRa response string
String buildResponse(unsigned long seqNum);

#endif
