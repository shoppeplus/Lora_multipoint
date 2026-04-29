#include "web_server.h"
#include <ESPAsyncWebServer.h>
#include "config.h"
#include "globals.h"
#include "baseline.h"
#include "wifi_manager.h"
#include "dashboard.h"

static AsyncWebServer server(80);

// Escape special chars in a string for safe JSON embedding
static String jsonEscape(const String& s) {
    String out;
    out.reserve(s.length() + 4);
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s.charAt(i);
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    return out;
}

void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", DASHBOARD_HTML);
    });

    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Snapshot shared state to avoid torn reads from async context
        SlaveData snap[NUM_SLAVES];
        int snapBaseCount[NUM_SLAVES];
        for (int i = 0; i < NUM_SLAVES; i++) {
            snap[i] = slaves[i];
            snapBaseCount[i] = baselines[i].count;
        }
        unsigned long snapCycle = cycleCount;
        bool snapBaseComplete = baselineComplete;
        String snapCalibResult = calibResult;

        String json = "{";
        json += "\"cycle\":" + String(snapCycle) + ",";
        json += "\"uptime\":" + String(millis() / 1000) + ",";
        json += "\"baselineReady\":" + String(snapBaseComplete ? "true" : "false") + ",";

        int minCount = BASELINE_CYCLES;
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (snapBaseCount[i] < minCount) minCount = snapBaseCount[i];
        }
        json += "\"baselineProgress\":" + String(minCount) + ",";
        json += "\"ntpTime\":\"" + getTimeString() + "\",";
        json += "\"apIP\":\"" + apIP + "\",";
        json += "\"staIP\":\"" + staIP + "\",";

        unsigned long totalP = 0, successP = 0;
        for (int i = 0; i < NUM_SLAVES; i++) {
            totalP += snap[i].totalPolls;
            successP += snap[i].successPolls;
        }
        json += "\"totalPolls\":" + String(totalP) + ",";
        json += "\"successPolls\":" + String(successP) + ",";

        json += "\"slaves\":[";
        for (int i = 0; i < NUM_SLAVES; i++) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"id\":" + String(i + 1) + ",";
            json += "\"online\":" + String(snap[i].online ? "true" : "false") + ",";
            json += "\"f1z\":" + String(snap[i].topFreqsZ[0], 2) + ",";
            json += "\"f2z\":" + String(snap[i].topFreqsZ[1], 2) + ",";
            json += "\"f3z\":" + String(snap[i].topFreqsZ[2], 2) + ",";
            json += "\"rmsZ\":" + String(snap[i].rmsZ, 4) + ",";
            json += "\"peakZ\":" + String(snap[i].peakZ, 4) + ",";
            json += "\"cfZ\":" + String(snap[i].crestFactorZ, 2) + ",";
            json += "\"freqX\":" + String(snap[i].freqX, 2) + ",";
            json += "\"rmsX\":" + String(snap[i].rmsX, 4) + ",";
            json += "\"peakX\":" + String(snap[i].peakX, 4) + ",";
            json += "\"freqY\":" + String(snap[i].freqY, 2) + ",";
            json += "\"rmsY\":" + String(snap[i].rmsY, 4) + ",";
            json += "\"peakY\":" + String(snap[i].peakY, 4) + ",";
            json += "\"alert\":" + String(snap[i].finalAlert) + ",";
            json += "\"rssi\":" + String(snap[i].rssi) + ",";
            json += "\"alertReasons\":\"" + jsonEscape(snap[i].alertReasons) + "\",";
            unsigned long offSec = 0;
            if (!snap[i].online && snap[i].lastSeen > 0) {
                offSec = (millis() - snap[i].lastSeen) / 1000;
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
        // Snapshot calibResult to avoid torn String read
        String snapResult = calibResult;
        String json = "{\"done\":" + String(calibDone ? "true" : "false") +
                      ",\"acks\":" + String(calibAckCount) +
                      ",\"total\":" + String(NUM_SLAVES) +
                      ",\"detail\":\"" + jsonEscape(snapResult) + "\"}";
        request->send(200, "application/json", json);
    });

    // History API: /api/history?slave=0&since=0
    // - First call (since=0): returns ALL points
    // - Subsequent (since=lastTimeSec): returns only NEW points
    server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        int si = 0;
        unsigned long since = 0;
        if (request->hasParam("slave")) {
            si = request->getParam("slave")->value().toInt();
        }
        if (request->hasParam("since")) {
            since = request->getParam("since")->value().toInt();
        }
        if (si < 0 || si >= NUM_SLAVES) si = 0;

        int count = historyCount[si];
        int head  = historyHead[si];

        String json = "{\"slave\":" + String(si + 1) +
                      ",\"count\":" + String(count) +
                      ",\"interval\":" + String(HISTORY_INTERVAL_S) +
                      ",\"max\":" + String(HISTORY_MAX) +
                      ",\"points\":[";

        int start = (count < HISTORY_MAX) ? 0 : head;
        bool first = true;
        int sent = 0;
        for (int k = 0; k < count; k++) {
            int idx = (start + k) % HISTORY_MAX;
            HistoryPoint& pt = history[si][idx];
            if (pt.timeSec <= since) continue; // Skip already-sent points
            if (!first) json += ",";
            json += "[" + String(pt.timeSec) + "," +
                    String(pt.rmsZ, 4) + "," +
                    String(pt.peakZ, 4) + "," +
                    String(pt.freqZ, 2) + "," +
                    String(pt.alert) + "]";
            first = false;
            sent++;
        }
        json += "],\"sent\":" + String(sent) + "}";
        request->send(200, "application/json", json);
    });

    server.begin();
    Serial.println("[WEB] Server started on port 80");
}
