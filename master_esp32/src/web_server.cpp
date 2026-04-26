#include "web_server.h"
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "globals.h"
#include "baseline.h"
#include "wifi_manager.h"
#include "dashboard.h"

static AsyncWebServer server(80);

void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", DASHBOARD_HTML);
    });

    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"cycle\":" + String(cycleCount) + ",";
        json += "\"uptime\":" + String(millis() / 1000) + ",";
        json += "\"baselineReady\":" + String(baselineComplete ? "true" : "false") + ",";

        int minCount = BASELINE_CYCLES;
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (baselines[i].count < minCount) minCount = baselines[i].count;
        }
        json += "\"baselineProgress\":" + String(minCount) + ",";
        json += "\"ntpTime\":\"" + getTimeString() + "\",";
        json += "\"apIP\":\"" + apIP + "\",";
        json += "\"staIP\":\"" + staIP + "\",";

        unsigned long totalP = 0, successP = 0;
        for (int i = 0; i < NUM_SLAVES; i++) {
            totalP += slaves[i].totalPolls;
            successP += slaves[i].successPolls;
        }
        json += "\"totalPolls\":" + String(totalP) + ",";
        json += "\"successPolls\":" + String(successP) + ",";

        json += "\"slaves\":[";
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"id\":" + String(i + 1) + ",";
            json += "\"online\":" + String(slaves[i].online ? "true" : "false") + ",";
            json += "\"f1z\":" + String(slaves[i].topFreqsZ[0], 2) + ",";
            json += "\"f2z\":" + String(slaves[i].topFreqsZ[1], 2) + ",";
            json += "\"f3z\":" + String(slaves[i].topFreqsZ[2], 2) + ",";
            json += "\"rmsZ\":" + String(slaves[i].rmsZ, 4) + ",";
            json += "\"peakZ\":" + String(slaves[i].peakZ, 4) + ",";
            json += "\"cfZ\":" + String(slaves[i].crestFactorZ, 2) + ",";
            json += "\"freqX\":" + String(slaves[i].freqX, 2) + ",";
            json += "\"rmsX\":" + String(slaves[i].rmsX, 4) + ",";
            json += "\"peakX\":" + String(slaves[i].peakX, 4) + ",";
            json += "\"freqY\":" + String(slaves[i].freqY, 2) + ",";
            json += "\"rmsY\":" + String(slaves[i].rmsY, 4) + ",";
            json += "\"peakY\":" + String(slaves[i].peakY, 4) + ",";
            json += "\"alert\":" + String(slaves[i].finalAlert) + ",";
            json += "\"rssi\":" + String(slaves[i].rssi) + ",";
            json += "\"alertReasons\":\"" + slaves[i].alertReasons + "\",";
            unsigned long offSec = 0;
            if (!slaves[i].online && slaves[i].lastSeen > 0) {
                offSec = (millis() - slaves[i].lastSeen) / 1000;
            }
            json += "\"lastSeen\":" + String(offSec);
            json += "}";
        }
        json += "]}";
        request->send(200, "application/json", json);
    });

    server.on("/api/reset-baseline", HTTP_POST, [](AsyncWebServerRequest *request) {
        resetBaselineData();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/api/recalibrate", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (needRecalibrate) {
            request->send(200, "application/json", "{\"status\":\"busy\"}");
            return;
        }
        needRecalibrate = true;
        calibDone = false;
        request->send(200, "application/json", "{\"status\":\"started\"}");
        Serial.println("[CMD] Recalibrate requested via dashboard");
    });

    server.on("/api/recalibrate-status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{\"done\":" + String(calibDone ? "true" : "false") +
                      ",\"acks\":" + String(calibAckCount) +
                      ",\"total\":" + String(NUM_SLAVES) +
                      ",\"detail\":\"" + calibResult + "\"}";
        request->send(200, "application/json", json);
    });

    server.begin();
    Serial.println("[WEB] Server started on port 80");
}
