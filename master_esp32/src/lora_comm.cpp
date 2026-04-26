#include "lora_comm.h"
#include <LoRa.h>
#include "config.h"
#include "globals.h"
#include "wifi_manager.h"

// ============================================
// Poll Slave (with retry)
// ============================================

bool pollSlave(uint8_t slaveId) {
    int idx = slaveId - 1;
    slaves[idx].totalPolls++;

    for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
        if (attempt > 0) {
            Serial.print("  Retry #");
            Serial.print(attempt);
            Serial.print("... ");
            delay(RETRY_DELAY_MS);
        }

        globalSeqNum++;
        String req = "REQ:" + String(slaveId) + ":" + String(globalSeqNum);
        slaves[idx].lastSeqSent = globalSeqNum;

        LoRa.beginPacket();
        LoRa.print(req);
        LoRa.endPacket();
        LoRa.receive();

        unsigned long startTime = millis();
        while (millis() - startTime < POLL_TIMEOUT_MS) {
            int packetSize = LoRa.parsePacket();
            if (packetSize > 0) {
                String response = "";
                while (LoRa.available()) {
                    response += (char)LoRa.read();
                }
                int rssi = LoRa.packetRssi();

                if (response.startsWith("DATA:")) {
                    String data = response.substring(5);

                    // Count commas to determine protocol version
                    int commas[14];
                    int commaCount = 0;
                    for (int i = 0; i < (int)data.length() && commaCount < 14; i++) {
                        if (data.charAt(i) == ',') {
                            commas[commaCount++] = i;
                        }
                    }

                    if (commaCount >= 7) {
                        int id = data.substring(0, commas[0]).toInt();

                        if (id == slaveId) {
                            if (commaCount == 14) {
                                // v3.2: id,seq,f1z,f2z,f3z,rmsZ,peakZ,cfZ,alert,f1x,rmsX,peakX,f1y,rmsY,peakY
                                slaves[idx].lastSeqRecv     = data.substring(commas[0] + 1, commas[1]).toInt();
                                slaves[idx].topFreqsZ[0]    = data.substring(commas[1] + 1, commas[2]).toFloat();
                                slaves[idx].topFreqsZ[1]    = data.substring(commas[2] + 1, commas[3]).toFloat();
                                slaves[idx].topFreqsZ[2]    = data.substring(commas[3] + 1, commas[4]).toFloat();
                                slaves[idx].rmsZ            = data.substring(commas[4] + 1, commas[5]).toFloat();
                                slaves[idx].peakZ           = data.substring(commas[5] + 1, commas[6]).toFloat();
                                slaves[idx].crestFactorZ    = data.substring(commas[6] + 1, commas[7]).toFloat();
                                slaves[idx].slaveAlert      = data.substring(commas[7] + 1, commas[8]).toInt();
                                slaves[idx].freqX           = data.substring(commas[8] + 1, commas[9]).toFloat();
                                slaves[idx].rmsX            = data.substring(commas[9] + 1, commas[10]).toFloat();
                                slaves[idx].peakX           = data.substring(commas[10] + 1, commas[11]).toFloat();
                                slaves[idx].freqY           = data.substring(commas[11] + 1, commas[12]).toFloat();
                                slaves[idx].rmsY            = data.substring(commas[12] + 1, commas[13]).toFloat();
                                slaves[idx].peakY           = data.substring(commas[13] + 1).toFloat();
                            } else if (commaCount == 10) {
                                // v3.0: id,seq,f1z,f2z,f3z,rmsZ,peakZ,cfZ,alert,rmsX,rmsY
                                slaves[idx].lastSeqRecv     = data.substring(commas[0] + 1, commas[1]).toInt();
                                slaves[idx].topFreqsZ[0]    = data.substring(commas[1] + 1, commas[2]).toFloat();
                                slaves[idx].topFreqsZ[1]    = data.substring(commas[2] + 1, commas[3]).toFloat();
                                slaves[idx].topFreqsZ[2]    = data.substring(commas[3] + 1, commas[4]).toFloat();
                                slaves[idx].rmsZ            = data.substring(commas[4] + 1, commas[5]).toFloat();
                                slaves[idx].peakZ           = data.substring(commas[5] + 1, commas[6]).toFloat();
                                slaves[idx].crestFactorZ    = data.substring(commas[6] + 1, commas[7]).toFloat();
                                slaves[idx].slaveAlert      = data.substring(commas[7] + 1, commas[8]).toInt();
                                slaves[idx].rmsX            = data.substring(commas[8] + 1, commas[9]).toFloat();
                                slaves[idx].rmsY            = data.substring(commas[9] + 1).toFloat();
                                slaves[idx].freqX = 0; slaves[idx].peakX = 0;
                                slaves[idx].freqY = 0; slaves[idx].peakY = 0;
                            } else {
                                // v2.0: id,f1z,f2z,f3z,rmsZ,peakZ,cfZ,alert
                                slaves[idx].topFreqsZ[0]    = data.substring(commas[0] + 1, commas[1]).toFloat();
                                slaves[idx].topFreqsZ[1]    = data.substring(commas[1] + 1, commas[2]).toFloat();
                                slaves[idx].topFreqsZ[2]    = data.substring(commas[2] + 1, commas[3]).toFloat();
                                slaves[idx].rmsZ            = data.substring(commas[3] + 1, commas[4]).toFloat();
                                slaves[idx].peakZ           = data.substring(commas[4] + 1, commas[5]).toFloat();
                                slaves[idx].crestFactorZ    = data.substring(commas[5] + 1, commas[6]).toFloat();
                                slaves[idx].slaveAlert      = data.substring(commas[6] + 1).toInt();
                                slaves[idx].rmsX = 0; slaves[idx].peakX = 0; slaves[idx].freqX = 0;
                                slaves[idx].rmsY = 0; slaves[idx].peakY = 0; slaves[idx].freqY = 0;
                            }

                            slaves[idx].rssi     = rssi;
                            slaves[idx].online   = true;
                            slaves[idx].lastSeen = millis();
                            slaves[idx].successPolls++;
                            return true;
                        }
                    }
                }
            }
            delay(1);
        }
    }

    slaves[slaveId - 1].online = false;
    return false;
}

// ============================================
// Sequential Recalibrate (WDT-safe)
// ============================================

void performRecalibrate() {
    Serial.println("[CMD] Recalibrating all slaves...");
    calibResult = "";
    calibAckCount = 0;

    for (int s = 1; s <= NUM_SLAVES; s++) {
        bool gotAck = false;

        for (int attempt = 0; attempt < 3 && !gotAck; attempt++) {
            if (attempt > 0) {
                Serial.print("[CMD] Retry S"); Serial.print(s);
                Serial.print(" #"); Serial.println(attempt);
                delay(500);
            }

            String cmd = "CALIB:" + String(s);
            LoRa.beginPacket();
            LoRa.print(cmd);
            LoRa.endPacket();
            LoRa.receive();

            Serial.print("[CMD] Sent "); Serial.print(cmd);
            Serial.print(" > waiting... ");

            unsigned long start = millis();
            while (millis() - start < 4000) {
                yield();  // Feed WDT
                int ps = LoRa.parsePacket();
                if (ps > 0) {
                    String msg = "";
                    while (LoRa.available()) msg += (char)LoRa.read();
                    if (msg.startsWith("ACK:")) {
                        int ackId = msg.substring(4, msg.indexOf(',')).toInt();
                        if (ackId == s) {
                            gotAck = true;
                            calibAckCount++;
                            if (calibResult.length() > 0) calibResult += " | ";
                            calibResult += "S" + String(s) + ":" + msg.substring(msg.lastIndexOf(',') + 1);
                            Serial.print("OK ("); Serial.print(msg); Serial.println(")");
                            break;
                        }
                    }
                }
                delay(10);
            }

            if (!gotAck && attempt < 2) Serial.println("TIMEOUT");
        }

        if (!gotAck) {
            Serial.print("[CMD] S"); Serial.print(s); Serial.println(" NO RESPONSE");
            if (calibResult.length() > 0) calibResult += " | ";
            calibResult += "S" + String(s) + ":FAIL";
        }

        if (s < NUM_SLAVES) delay(200);
        yield();
    }

    calibDone = true;
    needRecalibrate = false;
    Serial.print("[CMD] Done: "); Serial.print(calibAckCount);
    Serial.print("/"); Serial.print(NUM_SLAVES); Serial.println(" OK");
}

// ============================================
// Serial Monitor Output
// ============================================

void printResults() {
    int onlineCount = 0, warningCount = 0, criticalCount = 0;
    bool hasAlerts = false;

    Serial.println();
    Serial.println("========================================================================");
    Serial.print("  BRIDGE MONITOR v3.2 [Cycle #");
    Serial.print(cycleCount);
    Serial.print("]  Up: ");
    Serial.print(millis() / 1000);
    Serial.print("s");
    String t = getTimeString();
    if (t.length() > 0) { Serial.print("  | "); Serial.print(t); }
    Serial.println();

    Serial.print("  Mode: ");
    if (!baselineComplete) {
        int mc = BASELINE_CYCLES;
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (baselines[i].count < mc) mc = baselines[i].count;
        }
        Serial.print("LEARNING ("); Serial.print(mc); Serial.print("/");
        Serial.print(BASELINE_CYCLES); Serial.println(")");
    } else {
        Serial.println("MONITORING");
    }
    Serial.print("  Web: http://"); Serial.print(apIP);
    if (staIP.length() > 0) { Serial.print(" | http://"); Serial.print(staIP); }
    Serial.println();
    Serial.println("------------------------------------------------------------------------");

    for (int i = 0; i < NUM_SLAVES; i++) {
        Serial.print("  Slave "); Serial.print(i + 1); Serial.print(": ");

        if (slaves[i].online) {
            onlineCount++;
            Serial.print("Z: f1="); Serial.print(slaves[i].topFreqsZ[0], 2);
            Serial.print("  f2="); Serial.print(slaves[i].topFreqsZ[1], 2);
            Serial.print("  f3="); Serial.print(slaves[i].topFreqsZ[2], 2);
            Serial.println(" Hz");

            Serial.print("          Z: RMS="); Serial.print(slaves[i].rmsZ, 4);
            Serial.print(" | Peak="); Serial.print(slaves[i].peakZ, 4);
            Serial.print(" | CF="); Serial.print(slaves[i].crestFactorZ, 2);
            Serial.print(" | "); Serial.print(slaves[i].rssi); Serial.println(" dBm");

            Serial.print("          X: f1="); Serial.print(slaves[i].freqX, 2);
            Serial.print(" Hz | RMS="); Serial.print(slaves[i].rmsX, 4);
            Serial.print(" | Peak="); Serial.print(slaves[i].peakX, 4); Serial.println(" g");

            Serial.print("          Y: f1="); Serial.print(slaves[i].freqY, 2);
            Serial.print(" Hz | RMS="); Serial.print(slaves[i].rmsY, 4);
            Serial.print(" | Peak="); Serial.print(slaves[i].peakY, 4); Serial.println(" g");

            Serial.print("          Pkt: ");
            Serial.print(slaves[i].successPolls); Serial.print("/");
            Serial.print(slaves[i].totalPolls);
            if (slaves[i].totalPolls > 0) {
                Serial.print(" ("); Serial.print(slaves[i].successPolls * 100 / slaves[i].totalPolls);
                Serial.print("%)");
            }
            Serial.println();

            Serial.print("          ");
            if (slaves[i].finalAlert == ALERT_CRITICAL) {
                Serial.print("!!! CRITICAL !!! "); criticalCount++; hasAlerts = true;
            } else if (slaves[i].finalAlert == ALERT_WARNING) {
                Serial.print(">> WARNING << "); warningCount++; hasAlerts = true;
            } else {
                Serial.print("[OK] NORMAL ");
            }
            if (slaves[i].alertReasons.length() > 0) {
                Serial.print("| "); Serial.print(slaves[i].alertReasons);
            }
            Serial.println();
        } else {
            unsigned long off = slaves[i].lastSeen > 0 ? (millis() - slaves[i].lastSeen) / 1000 : 0;
            Serial.print("--- OFFLINE ---");
            if (off > 0) { Serial.print(" ("); Serial.print(off); Serial.print("s ago)"); }
            Serial.println();
        }
        if (i < NUM_SLAVES - 1) Serial.println();
    }

    Serial.println("------------------------------------------------------------------------");
    Serial.print("  "); Serial.print(onlineCount); Serial.print("/");
    Serial.print(NUM_SLAVES); Serial.print(" ONLINE");
    if (criticalCount > 0) { Serial.print(" | "); Serial.print(criticalCount); Serial.print(" CRIT"); }
    if (warningCount > 0) { Serial.print(" | "); Serial.print(warningCount); Serial.print(" WARN"); }
    if (!hasAlerts && onlineCount == NUM_SLAVES) Serial.print("  * ALL NORMAL *");
    Serial.println();
    Serial.println("========================================================================");
}
