#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include "config.h"

struct SlaveData {
    float topFreqsZ[NUM_TOP_FREQS];
    float rmsZ, peakZ, crestFactorZ;
    float freqX, rmsX, peakX;
    float freqY, rmsY, peakY;
    int   slaveAlert, masterAlert, finalAlert;
    int   rssi;
    bool  online;
    unsigned long lastSeen;
    String alertReasons;
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
