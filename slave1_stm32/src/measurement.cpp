#include "measurement.h"
#include "mpu6050.h"
#include "fft_analysis.h"
#include "calibration.h"

// Sample buffers
float samples[SAMPLE_SIZE];
#if ENABLE_MULTI_AXIS
float samplesX[SAMPLE_SIZE];
float samplesY[SAMPLE_SIZE];
#endif

// Raw results (before EMA)
static float rawFreqsZ[NUM_TOP_FREQS];
static float rawRmsZ = 0, rawPeakZ = 0, rawCfZ = 0;
static float rawFreqX = 0, rawRmsX = 0, rawPeakX = 0;
static float rawFreqY = 0, rawRmsY = 0, rawPeakY = 0;

// Smoothed results (after EMA, exported)
float topFreqsZ[NUM_TOP_FREQS] = {0};
float rmsZ = 0, peakZ = 0, crestFactorZ = 0;
float topFreqX = 0, rmsX = 0, peakX = 0;
float topFreqY = 0, rmsY = 0, peakY = 0;

int alertLevel = ALERT_NORMAL;
unsigned long measureCount = 0;
bool dataReady = false;
unsigned long lastMeasure = 0;

// EMA: output = α × new + (1-α) × prev
static float ema(float prev, float newVal, float alpha) {
    if (prev == 0) return newVal;
    return alpha * newVal + (1.0f - alpha) * prev;
}

void collectSamples() {
    unsigned long sampleInterval = 1000000UL / SAMPLE_RATE;

    for (int i = 0; i < SAMPLE_SIZE; i++) {
        unsigned long t = micros();
        float ax, ay, az;
        readAccelXYZ(ax, ay, az);
        float raw[3] = {ax, ay, az};

#if ENABLE_MULTI_AXIS
        samples[i]  = raw[gravAxis];
        samplesX[i] = raw[horzAxis1];
        samplesY[i] = raw[horzAxis2];
#else
        samples[i] = raw[gravAxis];
#endif
        while ((micros() - t) < sampleInterval) {}
    }
}

static void determineAlertLevel() {
    alertLevel = ALERT_NORMAL;

    if (rawPeakZ >= IMPACT_CRITICAL_G) alertLevel = ALERT_CRITICAL;
    else if (rawPeakZ >= IMPACT_WARNING_G) alertLevel = ALERT_WARNING;

    if (rawCfZ >= CF_CRITICAL) alertLevel = ALERT_CRITICAL;
    else if (rawCfZ >= CF_WARNING) alertLevel = max(alertLevel, ALERT_WARNING);

#if ENABLE_MULTI_AXIS
    if (rawPeakX >= IMPACT_CRITICAL_G || rawPeakY >= IMPACT_CRITICAL_G)
        alertLevel = max(alertLevel, ALERT_CRITICAL);
    else if (rawPeakX >= IMPACT_WARNING_G || rawPeakY >= IMPACT_WARNING_G)
        alertLevel = max(alertLevel, ALERT_WARNING);
#endif
}

void performMeasurement() {
    collectSamples();

    processAxisFFT(samples, rawFreqsZ, NUM_TOP_FREQS,
                   &rawRmsZ, &rawPeakZ, &rawCfZ);

#if ENABLE_MULTI_AXIS
    processAxisFFT(samplesX, &rawFreqX, NUM_TOP_FREQS_XY,
                   &rawRmsX, &rawPeakX, NULL);
    processAxisFFT(samplesY, &rawFreqY, NUM_TOP_FREQS_XY,
                   &rawRmsY, &rawPeakY, NULL);
#else
    rawRmsX = 0; rawPeakX = 0; rawFreqX = 0;
    rawRmsY = 0; rawPeakY = 0; rawFreqY = 0;
#endif

    determineAlertLevel();

    // EMA smoothing
    for (int i = 0; i < NUM_TOP_FREQS; i++)
        topFreqsZ[i] = ema(topFreqsZ[i], rawFreqsZ[i], EMA_ALPHA);
    rmsZ         = ema(rmsZ, rawRmsZ, EMA_ALPHA);
    peakZ        = ema(peakZ, rawPeakZ, EMA_ALPHA);
    crestFactorZ = ema(crestFactorZ, rawCfZ, EMA_ALPHA);
    topFreqX     = ema(topFreqX, rawFreqX, EMA_ALPHA);
    rmsX         = ema(rmsX, rawRmsX, EMA_ALPHA);
    peakX        = ema(peakX, rawPeakX, EMA_ALPHA);
    topFreqY     = ema(topFreqY, rawFreqY, EMA_ALPHA);
    rmsY         = ema(rmsY, rawRmsY, EMA_ALPHA);
    peakY        = ema(peakY, rawPeakY, EMA_ALPHA);

    dataReady = true;
    measureCount++;
}

void resetEMA() {
    for (int i = 0; i < NUM_TOP_FREQS; i++) topFreqsZ[i] = 0;
    rmsZ = 0; peakZ = 0; crestFactorZ = 0;
    topFreqX = 0; rmsX = 0; peakX = 0;
    topFreqY = 0; rmsY = 0; peakY = 0;
    dataReady = false;
}

String buildResponse(unsigned long seqNum) {
    // Format: DATA:id,seq,f1z,f2z,f3z,rmsZ,peakZ,cfZ,alert,f1x,rmsX,peakX,f1y,rmsY,peakY
    return "DATA:" + String(SLAVE_ID) + "," +
           String(seqNum) + "," +
           String(topFreqsZ[0], 2) + "," +
           String(topFreqsZ[1], 2) + "," +
           String(topFreqsZ[2], 2) + "," +
           String(rmsZ, 4) + "," +
           String(peakZ, 4) + "," +
           String(crestFactorZ, 2) + "," +
           String(alertLevel) + "," +
           String(topFreqX, 2) + "," +
           String(rmsX, 4) + "," +
           String(peakX, 4) + "," +
           String(topFreqY, 2) + "," +
           String(rmsY, 4) + "," +
           String(peakY, 4);
}
