#include "baseline.h"
#include <math.h>
#include "config.h"
#include "globals.h"

// ============================================
// NVS Persistence
// ============================================

void saveBaseline() {
    prefs.begin("baseline", false);
    prefs.putBool("complete", baselineComplete);
    for (int i = 0; i < NUM_SLAVES; i++) {
        String p = "s" + String(i);
        prefs.putBool((p + "r").c_str(), baselines[i].ready);
        prefs.putInt((p + "c").c_str(), baselines[i].count);
        prefs.putFloat((p + "fm").c_str(), baselines[i].f1_mean);
        prefs.putFloat((p + "fs").c_str(), baselines[i].f1_std);
        prefs.putFloat((p + "rm").c_str(), baselines[i].rms_mean);
        prefs.putFloat((p + "rs").c_str(), baselines[i].rms_std);
        prefs.putFloat((p + "pm").c_str(), baselines[i].peak_mean);
        prefs.putFloat((p + "ps").c_str(), baselines[i].peak_std);
    }
    prefs.end();
    Serial.println("[NVS] Baseline saved");
}

void loadBaseline() {
    prefs.begin("baseline", true);
    if (prefs.getBool("complete", false)) {
        baselineComplete = true;
        for (int i = 0; i < NUM_SLAVES; i++) {
            String p = "s" + String(i);
            baselines[i].ready     = prefs.getBool((p + "r").c_str(), false);
            baselines[i].count     = prefs.getInt((p + "c").c_str(), 0);
            baselines[i].f1_mean   = prefs.getFloat((p + "fm").c_str(), 0);
            baselines[i].f1_std    = prefs.getFloat((p + "fs").c_str(), 0.01f);
            baselines[i].rms_mean  = prefs.getFloat((p + "rm").c_str(), 0);
            baselines[i].rms_std   = prefs.getFloat((p + "rs").c_str(), 0.0001f);
            baselines[i].peak_mean = prefs.getFloat((p + "pm").c_str(), 0);
            baselines[i].peak_std  = prefs.getFloat((p + "ps").c_str(), 0.0001f);
        }
        Serial.println("[NVS] Baseline loaded, anomaly detection ACTIVE");
    } else {
        Serial.println("[NVS] No saved baseline");
    }
    prefs.end();
}

void resetBaselineData() {
    baselineComplete = false;
    for (int i = 0; i < NUM_SLAVES; i++) {
        baselines[i].count = 0;
        baselines[i].ready = false;
    }
    prefs.begin("baseline", false);
    prefs.clear();
    prefs.end();
    Serial.println("[NVS] Baseline RESET");
}

// ============================================
// Baseline Learning
// ============================================

void updateBaseline(int slaveIdx) {
    BaselineData& b = baselines[slaveIdx];
    SlaveData& s = slaves[slaveIdx];
    if (b.ready || !s.online) return;

    int idx = b.count;
    if (idx < BASELINE_CYCLES) {
        b.f1_history[idx]   = s.topFreqsZ[0];
        b.rms_history[idx]  = s.rmsZ;
        b.peak_history[idx] = s.peakZ;
        b.count++;
    }

    if (b.count >= BASELINE_CYCLES) {
        // Compute mean
        b.f1_mean = 0; b.rms_mean = 0; b.peak_mean = 0;
        for (int i = 0; i < BASELINE_CYCLES; i++) {
            b.f1_mean   += b.f1_history[i];
            b.rms_mean  += b.rms_history[i];
            b.peak_mean += b.peak_history[i];
        }
        b.f1_mean /= BASELINE_CYCLES;
        b.rms_mean /= BASELINE_CYCLES;
        b.peak_mean /= BASELINE_CYCLES;

        // Compute std deviation
        b.f1_std = 0; b.rms_std = 0; b.peak_std = 0;
        for (int i = 0; i < BASELINE_CYCLES; i++) {
            float d;
            d = b.f1_history[i] - b.f1_mean;     b.f1_std += d * d;
            d = b.rms_history[i] - b.rms_mean;    b.rms_std += d * d;
            d = b.peak_history[i] - b.peak_mean;  b.peak_std += d * d;
        }
        b.f1_std   = sqrtf(b.f1_std / BASELINE_CYCLES);
        b.rms_std  = sqrtf(b.rms_std / BASELINE_CYCLES);
        b.peak_std = sqrtf(b.peak_std / BASELINE_CYCLES);

        // Clamp minimum std to avoid division by zero
        if (b.f1_std < 0.01f)     b.f1_std = 0.01f;
        if (b.rms_std < 0.0001f)  b.rms_std = 0.0001f;
        if (b.peak_std < 0.0001f) b.peak_std = 0.0001f;

        b.ready = true;
        Serial.print("[BASELINE] S");
        Serial.print(slaveIdx + 1);
        Serial.print(" COMPLETE: f1=");
        Serial.print(b.f1_mean, 2);
        Serial.print(" +/- ");
        Serial.print(b.f1_std, 2);
        Serial.println(" Hz");
    }
}

// ============================================
// 3-Sigma Anomaly Detection
// ============================================

int checkAnomaly(int slaveIdx) {
    BaselineData& b = baselines[slaveIdx];
    SlaveData& s = slaves[slaveIdx];
    if (!b.ready || !s.online) return ALERT_NORMAL;

    int alert = ALERT_NORMAL;
    s.alertReasons = "";

    // Frequency shift check
    float f1_dev = fabsf(s.topFreqsZ[0] - b.f1_mean) / b.f1_std;
    if (f1_dev > ANOMALY_SIGMA * 2) {
        alert = ALERT_CRITICAL;
        s.alertReasons += "FREQ_SHIFT(" + String(f1_dev, 1) + "s) ";
    } else if (f1_dev > ANOMALY_SIGMA) {
        alert = max(alert, ALERT_WARNING);
        s.alertReasons += "freq(" + String(f1_dev, 1) + "s) ";
    }

    // RMS vibration check
    float rms_dev = fabsf(s.rmsZ - b.rms_mean) / b.rms_std;
    if (rms_dev > ANOMALY_SIGMA * 2) {
        alert = max(alert, ALERT_CRITICAL);
        s.alertReasons += "HIGH_VIB(" + String(rms_dev, 1) + "s) ";
    } else if (rms_dev > ANOMALY_SIGMA) {
        alert = max(alert, ALERT_WARNING);
        s.alertReasons += "vib(" + String(rms_dev, 1) + "s) ";
    }

    // Peak impact check
    float peak_dev = fabsf(s.peakZ - b.peak_mean) / b.peak_std;
    if (peak_dev > ANOMALY_SIGMA * 2) {
        alert = max(alert, ALERT_CRITICAL);
        s.alertReasons += "IMPACT(" + String(peak_dev, 1) + "s) ";
    } else if (peak_dev > ANOMALY_SIGMA) {
        alert = max(alert, ALERT_WARNING);
        s.alertReasons += "impact(" + String(peak_dev, 1) + "s) ";
    }

    return alert;
}
