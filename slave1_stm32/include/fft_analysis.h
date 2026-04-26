#ifndef FFT_ANALYSIS_H
#define FFT_ANALYSIS_H

#include <Arduino.h>
#include "config.h"

// Shared FFT buffers
extern float vReal[SAMPLE_SIZE];
extern float vImag[SAMPLE_SIZE];
extern float hannWindow[SAMPLE_SIZE];

// Pre-compute Hann window coefficients
void initHannWindow();

// Full axis processing: DC removal → RMS/Peak/CF → Hann → FFT → peak finding
void processAxisFFT(float* axisSamples, float* outFreqs, int numFreqs,
                    float* outRms, float* outPeak, float* outCF);

#endif
