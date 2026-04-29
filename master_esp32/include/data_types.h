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

// 20 bytes per point — compact for ring buffer
struct HistoryPoint {
    uint32_t timeSec;       // Uptime in seconds
    float    rmsZ;          // RMS vibration (g)
    float    peakZ;         // Peak vibration (g)
    float    freqZ;         // Dominant frequency (Hz)
    uint8_t  alert;         // Alert level (0/1/2)
    uint8_t  _pad[3];       // Alignment padding
};

#endif
