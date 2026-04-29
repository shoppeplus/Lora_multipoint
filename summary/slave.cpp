/* 
 * MONOLITHIC MERGE OF d:\TIm\slave1_stm32
 */

#include <Arduino.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>


// ============================================
// FILE: config.h
// ============================================

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Slave STM32 — Bridge Vibration Monitor
// ============================================

// --- Slave ID (change per device: 1 or 2) ---
#define SLAVE_ID 1

// --- LoRa RA-02 (SX1278) — SPI1 ---
#define LORA_SS PA4
#define LORA_RST PB9
#define LORA_DIO0 PB8
#define LORA_SCK PA5
#define LORA_MISO PA6
#define LORA_MOSI PA7

// --- MPU6050 — I2C1 ---
#define MPU_SDA PB7
#define MPU_SCL PB6
#define MPU_ADDR 0x68
#define PWR_MGMT_1 0x6B
#define ACCEL_XOUT_H 0x3B
#define ACCEL_CONFIG 0x1C
#define SMPLRT_DIV 0x19
#define MPU_CONFIG 0x1A

// --- LoRa RF (must match Master) ---
#define LORA_FREQ 433E6
#define LORA_SF 7
#define LORA_BW 125E3
#define LORA_CR 5
#define LORA_TX_POWER 17

// --- FFT ---
#define SAMPLE_SIZE 256
#define SAMPLE_RATE 1000 // Hz

// --- Multi-Frequency Peak Detection ---
#define NUM_TOP_FREQS 3     // Top 3 peaks for Z-axis
#define NUM_TOP_FREQS_XY 1  // Top 1 peak for X/Y axes
#define MIN_PEAK_DISTANCE 3 // Min bin gap between peaks

// --- Multi-Axis ---
#define ENABLE_MULTI_AXIS 1

// --- Noise Filtering ---
#define NOISE_FLOOR_MAG 5.0f // FFT magnitude threshold
#define EMA_ALPHA 0.3f       // Smoothing factor (0.3 = 30% new)

// --- Auto-Orientation Calibration ---
#define CALIB_SAMPLES 100      // Samples to detect gravity axis
#define GRAVITY_THRESHOLD 0.7f // Min |mean| to confirm gravity (g)

// --- Impact Thresholds (AC values, after DC removal) ---
#define IMPACT_WARNING_G 0.1f
#define IMPACT_CRITICAL_G 0.5f
#define CF_WARNING 5.0f
#define CF_CRITICAL 10.0f

// --- Alert Levels ---
#define ALERT_NORMAL 0
#define ALERT_WARNING 1
#define ALERT_CRITICAL 2

// --- Timing ---
#define MEASURE_INTERVAL_MS 2000 // Fallback measurement interval
#define SERIAL_BAUD 115200

#endif

// ============================================
// FILE: calibration.h
// ============================================

#ifndef CALIBRATION_H
#define CALIBRATION_H


// Axis mapping (set by calibrateOrientation)
extern int gravAxis;       // Physical axis with gravity → logical Z
extern int horzAxis1;      // Horizontal axis 1 → logical X
extern int horzAxis2;      // Horizontal axis 2 → logical Y
extern const char* axisNames[];

// Detect gravity axis from 100 samples, remap logical axes
void calibrateOrientation();

#endif

// ============================================
// FILE: fft_analysis.h
// ============================================

#ifndef FFT_ANALYSIS_H
#define FFT_ANALYSIS_H


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

// ============================================
// FILE: lora_handler.h
// ============================================

#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H


// Handle incoming LoRa packets (REQ polling + CALIB commands)
bool checkAndRespond();

#endif

// ============================================
// FILE: measurement.h
// ============================================

#ifndef MEASUREMENT_H
#define MEASUREMENT_H


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

// ============================================
// FILE: mpu6050.h
// ============================================

#ifndef MPU6050_H
#define MPU6050_H


void mpuInit();
bool readAccelXYZ(float &ax, float &ay, float &az);

#endif

// ============================================
// FILE: calibration.cpp
// ============================================


// Default axis mapping: physical Z = vertical
int gravAxis  = 2;
int horzAxis1 = 0;
int horzAxis2 = 1;
const char* axisNames[] = {"X", "Y", "Z"};

void calibrateOrientation() {
    float sum[3] = {0, 0, 0};
    int validCount = 0;

    for (int i = 0; i < CALIB_SAMPLES; i++) {
        float ax, ay, az;
        if (readAccelXYZ(ax, ay, az)) {
            sum[0] += ax;
            sum[1] += ay;
            sum[2] += az;
            validCount++;
        }
        delay(5);
    }

    if (validCount == 0) {
        Serial.println("[CALIB] ERROR: No valid I2C reads! Using default Z");
        gravAxis = 2; horzAxis1 = 0; horzAxis2 = 1;
        return;
    }

    float mean[3];
    float absMax = 0;
    int maxAxis = 2;

    for (int i = 0; i < 3; i++) {
        mean[i] = sum[i] / validCount;
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

// ============================================
// FILE: fft_analysis.cpp
// ============================================


float vReal[SAMPLE_SIZE];
float vImag[SAMPLE_SIZE];
float hannWindow[SAMPLE_SIZE];

void initHannWindow() {
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        hannWindow[i] = 0.5f * (1.0f - cosf(2.0f * PI * i / (SAMPLE_SIZE - 1)));
    }
}

// Cooley-Tukey Radix-2 in-place FFT
static void fft(float* real, float* imag, int n) {
    // Bit-reversal permutation
    int j = 0;
    for (int i = 1; i < n - 1; i++) {
        int bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) {
            float temp = real[i]; real[i] = real[j]; real[j] = temp;
            temp = imag[i]; imag[i] = imag[j]; imag[j] = temp;
        }
    }

    // Butterfly stages
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * PI / len;
        float wReal = cosf(angle);
        float wImag = sinf(angle);
        for (int i = 0; i < n; i += len) {
            float curReal = 1.0f, curImag = 0.0f;
            for (int k = 0; k < len / 2; k++) {
                int idx1 = i + k;
                int idx2 = i + k + len / 2;
                float tReal = curReal * real[idx2] - curImag * imag[idx2];
                float tImag = curReal * imag[idx2] + curImag * real[idx2];
                real[idx2] = real[idx1] - tReal;
                imag[idx2] = imag[idx1] - tImag;
                real[idx1] += tReal;
                imag[idx1] += tImag;
                float newCurReal = curReal * wReal - curImag * wImag;
                curImag = curReal * wImag + curImag * wReal;
                curReal = newCurReal;
            }
        }
    }
}

void processAxisFFT(float* axisSamples, float* outFreqs, int numFreqs,
                    float* outRms, float* outPeak, float* outCF) {
    // 1. DC removal
    float mean = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) mean += axisSamples[i];
    mean /= SAMPLE_SIZE;

    // 2. RMS + Peak (on AC signal)
    float rmsVal = 0;
    float peakVal = 0;
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        axisSamples[i] -= mean;
        float absVal = fabsf(axisSamples[i]);
        if (absVal > peakVal) peakVal = absVal;
        rmsVal += axisSamples[i] * axisSamples[i];
    }
    rmsVal = sqrtf(rmsVal / SAMPLE_SIZE);
    *outRms = rmsVal;
    *outPeak = peakVal;

    // 3. Crest Factor
    if (outCF != NULL) {
        *outCF = (rmsVal > 0.0001f) ? (peakVal / rmsVal) : 0;
    }

    // 4. Hann window + FFT
    for (int i = 0; i < SAMPLE_SIZE; i++) {
        vReal[i] = axisSamples[i] * hannWindow[i];
        vImag[i] = 0;
    }
    fft(vReal, vImag, SAMPLE_SIZE);

    // 5. Magnitude spectrum
    int halfSize = SAMPLE_SIZE / 2;
    for (int i = 0; i < halfSize; i++) {
        vReal[i] = sqrtf(vReal[i] * vReal[i] + vImag[i] * vImag[i]);
    }

    // 6. Find top frequency peaks (above noise floor, min distance apart)
    int freqBins[NUM_TOP_FREQS];
    for (int p = 0; p < numFreqs; p++) {
        float maxMag = 0;
        int maxIdx = 0;

        for (int i = 1; i < halfSize; i++) {
            if (vReal[i] > maxMag) {
                bool tooClose = false;
                for (int q = 0; q < p; q++) {
                    if (abs(i - freqBins[q]) < MIN_PEAK_DISTANCE) {
                        tooClose = true;
                        break;
                    }
                }
                if (!tooClose) {
                    maxMag = vReal[i];
                    maxIdx = i;
                }
            }
        }

        if (maxMag >= NOISE_FLOOR_MAG) {
            outFreqs[p] = maxIdx * ((float)SAMPLE_RATE / SAMPLE_SIZE);
        } else {
            outFreqs[p] = 0;
        }
        freqBins[p] = maxIdx;
    }
}

// ============================================
// FILE: lora_handler.cpp
// ============================================


bool checkAndRespond() {
    int packetSize = LoRa.parsePacket();
    if (packetSize <= 0) return false;

    String request = "";
    while (LoRa.available()) {
        request += (char)LoRa.read();
    }

    // --- CALIB command: "CALIB:<id>" (0 = broadcast) ---
    if (request.startsWith("CALIB:")) {
        int calibId = request.substring(6).toInt();
        if (calibId == 0 || calibId == SLAVE_ID) {
            Serial.println();
            Serial.println("[CALIB] Recalibrate command received");
            calibrateOrientation();
            resetEMA();

            // Warm up EMA with 3 measurements
            for (int i = 0; i < 3; i++) {
                performMeasurement();
                delay(50);
            }

            // Send ACK with detected gravity axis
            String ack = "ACK:" + String(SLAVE_ID) + ",CALIB," + String(axisNames[gravAxis]);
            LoRa.beginPacket();
            LoRa.print(ack);
            LoRa.endPacket();
            LoRa.receive();

            Serial.print("[CALIB] Done, gravity=");
            Serial.print(axisNames[gravAxis]);
            Serial.println(", ACK sent");
            Serial.println();
        }
        return true;
    }

    // --- REQ command: "REQ:<id>:<seq>" ---
    if (!request.startsWith("REQ:")) return false;

    String payload = request.substring(4);
    int colonIdx = payload.indexOf(':');
    int reqId;
    unsigned long seqNum = 0;

    if (colonIdx > 0) {
        reqId = payload.substring(0, colonIdx).toInt();
        seqNum = payload.substring(colonIdx + 1).toInt();
    } else {
        reqId = payload.toInt();
    }

    if (reqId != SLAVE_ID) return false;

    Serial.print("[RX] REQ #");
    Serial.print(seqNum);

    // Step 1: Respond immediately with cached data
    if (dataReady) {
        String response = buildResponse(seqNum);
        LoRa.beginPacket();
        LoRa.print(response);
        LoRa.endPacket();

        Serial.print(" > TX (V:");
        Serial.print(topFreqsZ[0], 1);
        Serial.print(" pk=");
        Serial.print(peakZ, 4);
        Serial.print(" A=");
        Serial.print(alertLevel);
        Serial.print(")");
    } else {
        Serial.print(" > first...");
        performMeasurement();
        String response = buildResponse(seqNum);
        LoRa.beginPacket();
        LoRa.print(response);
        LoRa.endPacket();
        Serial.print(" TX");
    }

    // Step 2: Measure fresh data for next poll cycle
    unsigned long t0 = millis();
    performMeasurement();
    unsigned long dt = millis() - t0;
    lastMeasure = millis();

    LoRa.receive();

    Serial.print(" > Measured ");
    Serial.print(dt);
    Serial.println("ms");

    return true;
}

// ============================================
// FILE: measurement.cpp
// ============================================


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
        if (readAccelXYZ(ax, ay, az)) {
            float raw[3] = {ax, ay, az};
#if ENABLE_MULTI_AXIS
            samples[i]  = raw[gravAxis];
            samplesX[i] = raw[horzAxis1];
            samplesY[i] = raw[horzAxis2];
#else
            samples[i] = raw[gravAxis];
#endif
        } else {
            // I2C error: repeat previous sample to avoid garbage
            samples[i]  = (i > 0) ? samples[i - 1] : 0;
#if ENABLE_MULTI_AXIS
            samplesX[i] = (i > 0) ? samplesX[i - 1] : 0;
            samplesY[i] = (i > 0) ? samplesY[i - 1] : 0;
#endif
        }
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

// ============================================
// FILE: mpu6050.cpp
// ============================================


static void mpuWriteReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

void mpuInit() {
    mpuWriteReg(PWR_MGMT_1, 0x00);
    delay(100);
    mpuWriteReg(ACCEL_CONFIG, 0x00);  // +/-2g
    mpuWriteReg(SMPLRT_DIV, 0x00);    // 1000 Hz
    mpuWriteReg(MPU_CONFIG, 0x03);    // DLPF ~44Hz
}

bool readAccelXYZ(float &ax, float &ay, float &az) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(ACCEL_XOUT_H);
    if (Wire.endTransmission(false) != 0) return false;

    uint8_t received = Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)6);
    if (received != 6) return false;

    int16_t rawX = ((int16_t)Wire.read() << 8) | Wire.read();
    int16_t rawY = ((int16_t)Wire.read() << 8) | Wire.read();
    int16_t rawZ = ((int16_t)Wire.read() << 8) | Wire.read();

    ax = rawX / 16384.0f;
    ay = rawY / 16384.0f;
    az = rawZ / 16384.0f;
    return true;
}

// ============================================
// FILE: main.cpp
// ============================================

/**
 * ============================================
 * SLAVE STM32 — Bridge Vibration Monitor v3.2
 * ============================================
 *
 * Architecture: Always-On + Respond-First
 * - Listens for LoRa polls, responds with cached data
 * - Measures fresh data after each response
 * - Fallback: self-measure every 2s if idle
 */


void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);

    Serial.println();
    Serial.println("========================================");
    Serial.print("  Bridge Monitor v3.2 — Slave ");
    Serial.println(SLAVE_ID);
    Serial.println("  Auto-Orientation + Continuous FFT");
    Serial.println("========================================");
    Serial.println();

    // I2C + MPU6050
    Wire.setSDA(MPU_SDA);
    Wire.setSCL(MPU_SCL);
    Wire.begin();
    Wire.setClock(400000);
    mpuInit();
    Serial.println("[INIT] MPU6050 OK");

    // Auto-Orientation
    Serial.println("[CALIB] Detecting gravity axis...");
    calibrateOrientation();
    Serial.println();

    // LoRa
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    Serial.print("[INIT] LoRa 433MHz... ");
    if (!LoRa.begin(LORA_FREQ)) {
        Serial.println("FAILED!");
        while (1) { delay(1000); }
    }
    LoRa.setSpreadingFactor(LORA_SF);
    LoRa.setSignalBandwidth(LORA_BW);
    LoRa.setCodingRate4(LORA_CR);
    LoRa.setTxPower(LORA_TX_POWER);
    LoRa.enableCrc();
    Serial.println("OK");

    // FFT + warm up
    initHannWindow();
    Serial.print("[INIT] Warming up... ");
    for (int i = 0; i < 3; i++) {
        performMeasurement();
        delay(50);
    }
    Serial.println("OK");

    Serial.print("[INIT] V: f1=");
    Serial.print(topFreqsZ[0], 2);
    Serial.print("Hz rms=");
    Serial.print(rmsZ, 4);
    Serial.print("g pk=");
    Serial.print(peakZ, 4);
    Serial.println("g");

    LoRa.receive();
    Serial.print("[READY] Gravity=");
    Serial.print(axisNames[gravAxis]);
    Serial.println(" | Listening for Master polls");
    Serial.println();
}

void loop() {
    // Priority 1: respond to LoRa polls
    checkAndRespond();

    // Priority 2: fallback measurement when idle
    if (millis() - lastMeasure >= MEASURE_INTERVAL_MS) {
        lastMeasure = millis();
        performMeasurement();
        LoRa.receive();

        if (measureCount % 10 == 0) {
            Serial.print("[IDLE #");
            Serial.print(measureCount);
            Serial.print("] V:");
            Serial.print(topFreqsZ[0], 1);
            Serial.print("Hz pk=");
            Serial.print(peakZ, 4);
            Serial.print(" alert=");
            Serial.println(alertLevel);
        }
    }
}
