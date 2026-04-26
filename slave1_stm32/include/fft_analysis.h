#ifndef FFT_ANALYSIS_H
#define FFT_ANALYSIS_H

#include <Arduino.h>
#include "config.h"

// FFT buffers (shared)
extern float vReal[SAMPLE_SIZE];
extern float vImag[SAMPLE_SIZE];
extern float hannWindow[SAMPLE_SIZE];

// Khởi tạo Hann window
void initHannWindow();

// FFT + peak detection + RMS/Peak/CF (tất cả sau DC removal)
void processAxisFFT(float* axisSamples, float* outFreqs, int numFreqs,
                    float* outRms, float* outPeak, float* outCF);

#endif
