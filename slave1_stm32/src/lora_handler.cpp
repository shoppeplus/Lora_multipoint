#include "lora_handler.h"
#include <LoRa.h>
#include "config.h"
#include "calibration.h"
#include "measurement.h"

bool checkAndRespond() {
    int packetSize = LoRa.parsePacket();
    if (packetSize <= 0) return false;

    String request = "";
    while (LoRa.available()) {
        request += (char)LoRa.read();
    }

    // === CALIB command ===
    if (request.startsWith("CALIB:")) {
        int calibId = request.substring(6).toInt();
        if (calibId == 0 || calibId == SLAVE_ID) {
            Serial.println();
            Serial.println("[CALIB] Recalibrate command received!");
            calibrateOrientation();
            resetEMA();

            for (int i = 0; i < 3; i++) {
                performMeasurement();
                delay(50);
            }

            String ack = "ACK:" + String(SLAVE_ID) + ",CALIB," + String(axisNames[gravAxis]);
            LoRa.beginPacket();
            LoRa.print(ack);
            LoRa.endPacket();
            LoRa.receive();

            Serial.print("[CALIB] Done! Gravity=");
            Serial.print(axisNames[gravAxis]);
            Serial.println(" → ACK sent");
            Serial.println();
        }
        return true;
    }

    // === REQ command ===
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

    // 1. Respond immediately with cached data
    if (dataReady) {
        String response = buildResponse(seqNum);
        LoRa.beginPacket();
        LoRa.print(response);
        LoRa.endPacket();

        Serial.print(" → TX (V:");
        Serial.print(topFreqsZ[0], 1);
        Serial.print(" pk=");
        Serial.print(peakZ, 4);
        Serial.print(" A=");
        Serial.print(alertLevel);
        Serial.print(")");
    } else {
        Serial.print(" → first...");
        performMeasurement();
        String response = buildResponse(seqNum);
        LoRa.beginPacket();
        LoRa.print(response);
        LoRa.endPacket();
        Serial.print(" TX");
    }

    // 2. Measure fresh data for next poll
    unsigned long t0 = millis();
    performMeasurement();
    unsigned long dt = millis() - t0;
    lastMeasure = millis();

    LoRa.receive();

    Serial.print(" → Measured ");
    Serial.print(dt);
    Serial.println("ms");

    return true;
}
