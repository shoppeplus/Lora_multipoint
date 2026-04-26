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
