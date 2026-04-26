#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <Arduino.h>
#include "config.h"

struct SlaveData {
    // Z-axis (vertical)
    float topFreqsZ[NUM_TOP_FREQS];
    float rmsZ, peakZ, crestFactorZ;
    // X-axis (horizontal 1)
    float freqX, rmsX, peakX;
    // Y-axis (horizontal 2)
    float freqY, rmsY, peakY;
    // Alert
    int   slaveAlert, masterAlert, finalAlert;
    String alertReasons;
    // Connectivity
    int   rssi;
    bool  online;
    unsigned long lastSeen;
    // Packet statistics
    unsigned long totalPolls, successPolls;
    unsigned long lastSeqSent, lastSeqRecv;
};

struct BaselineData {
    float f1_history[BASELINE_CYCLES];
    float rms_history[BASELINE_CYCLES];
    float peak_history[BASELINE_CYCLES];
    int   count;
    bool  ready;
    float f1_mean, f1_std;
    float rms_mean, rms_std;
    float peak_mean, peak_std;
};

#endif
