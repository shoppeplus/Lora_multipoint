/**
 * ============================================
 * SLAVE STM32 - Bridge Vibration Monitor v3.2
 * ============================================
 * 
 * Modules:
 *   mpu6050     — Driver cảm biến gia tốc
 *   calibration — Tự phát hiện trục gravity
 *   fft_analysis— FFT Cooley-Tukey + peak detection
 *   measurement — Thu thập mẫu, xử lý, EMA smoothing
 *   lora_handler— Xử lý REQ/CALIB từ Master
 */

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LoRa.h>
#include "config.h"
#include "mpu6050.h"
#include "calibration.h"
#include "fft_analysis.h"
#include "measurement.h"
#include "lora_handler.h"

// ============================================
// Setup
// ============================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);

    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.print("║  Bridge Monitor v3.2 - Slave ");
    Serial.print(SLAVE_ID);
    Serial.println("         ║");
    Serial.println("║  Auto-Orientation + Continuous FFT     ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();

    // I2C + MPU6050
    Wire.setSDA(MPU_SDA);
    Wire.setSCL(MPU_SCL);
    Wire.begin();
    Wire.setClock(400000);
    mpuInit();
    Serial.println("[INIT] MPU6050 OK (±2g, 1000Hz, DLPF 44Hz)");

    // Auto-Orientation
    Serial.println();
    Serial.println("[CALIB] Detecting gravity axis...");
    calibrateOrientation();
    Serial.println();

    // LoRa
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    Serial.print("[INIT] LoRa 433 MHz... ");
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

    // FFT window + warm up
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
    Serial.println();
    Serial.print("[READY] Gravity=phys.");
    Serial.print(axisNames[gravAxis]);
    Serial.println(" | AC values (gravity removed)");
    Serial.println();
}

// ============================================
// Main Loop
// ============================================
void loop() {
    // Ưu tiên 1: lắng nghe LoRa
    checkAndRespond();

    // Ưu tiên 2: fallback measurement (nếu không có poll)
    if (millis() - lastMeasure >= MEASURE_INTERVAL_MS) {
        lastMeasure = millis();
        performMeasurement();
        LoRa.receive();

        if (measureCount % 10 == 0) {
            Serial.print("[IDLE M#");
            Serial.print(measureCount);
            Serial.print("] V:");
            Serial.print(topFreqsZ[0], 1);
            Serial.print("Hz pk=");
            Serial.print(peakZ, 4);
            Serial.print(" A=");
            Serial.println(alertLevel);
        }
    }
}
