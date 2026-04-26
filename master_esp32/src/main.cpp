/**
 * ============================================
 * MASTER ESP32 — Bridge Vibration Monitor v3.2
 * ============================================
 *
 * Architecture: Always-On polling + Web Dashboard
 * - Polls 2 slaves via LoRa every ~1s
 * - Baseline learning + 3-sigma anomaly detection
 * - WiFi AP+STA with async web dashboard
 */

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "config.h"
#include "globals.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "lora_comm.h"
#include "baseline.h"

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("  BRIDGE VIBRATION MONITOR v3.2 MASTER");
    Serial.println("  Always-On + Multi-Axis + Dashboard");
    Serial.println("========================================");
    Serial.println();

    // LoRa
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
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

    // Init slave data
    for (int i = 0; i < NUM_SLAVES; i++) {
        slaves[i] = {};
        baselines[i] = {};
    }

    // Network + Web
    setupWiFi();
    setupNTP();
    setupWebServer();
    loadBaseline();

    Serial.println("[INIT] Ready");
    Serial.println();
}

void loop() {
    // Handle recalibrate request (non-blocking, WDT-safe)
    if (needRecalibrate) {
        performRecalibrate();
        return;
    }

    cycleCount++;

    // Poll all slaves
    for (int i = 1; i <= NUM_SLAVES; i++) {
        Serial.print("  Poll S"); Serial.print(i); Serial.print("... ");
        if (pollSlave(i)) {
            Serial.print("OK (Z:");
            Serial.print(slaves[i - 1].topFreqsZ[0], 1);
            Serial.print(" X:");
            Serial.print(slaves[i - 1].freqX, 1);
            Serial.print(" Y:");
            Serial.print(slaves[i - 1].freqY, 1);
            Serial.println(" Hz)");
        } else {
            Serial.println("FAIL");
        }
        if (i < NUM_SLAVES) delay(GUARD_TIME_MS);
    }

    // Baseline learning
    for (int i = 0; i < NUM_SLAVES; i++) updateBaseline(i);

    if (!baselineComplete) {
        bool allReady = true;
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (!baselines[i].ready) { allReady = false; break; }
        }
        if (allReady) {
            baselineComplete = true;
            saveBaseline();
            Serial.println("* BASELINE COMPLETE — Anomaly detection ACTIVE *");
        }
    }

    // Anomaly detection
    for (int i = 0; i < NUM_SLAVES; i++) {
        if (slaves[i].online) {
            slaves[i].masterAlert = checkAnomaly(i);
            slaves[i].finalAlert = max(slaves[i].slaveAlert, slaves[i].masterAlert);
        }
    }

    printResults();
    delay(POLL_INTERVAL_MS);
}
