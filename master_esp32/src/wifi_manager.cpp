#include "wifi_manager.h"
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "globals.h"

void setupWiFi() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
    delay(500);
    apIP = WiFi.softAPIP().toString();
    Serial.print("[WIFI] AP: ");
    Serial.print(WIFI_AP_SSID);
    Serial.print(" > http://");
    Serial.println(apIP);

    String ssid = WIFI_STA_SSID;
    if (ssid.length() > 0) {
        Serial.print("[WIFI] STA > ");
        Serial.print(WIFI_STA_SSID);
        WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
            staIP = WiFi.localIP().toString();
            Serial.print(" OK > http://");
            Serial.println(staIP);
        } else {
            Serial.println(" FAILED (AP-only)");
            WiFi.mode(WIFI_AP);
            WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
            delay(500);
            apIP = WiFi.softAPIP().toString();
        }
    } else {
        Serial.println("[WIFI] STA not configured, AP-only");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
        delay(500);
        apIP = WiFi.softAPIP().toString();
    }
}

void setupNTP() {
    if (WiFi.status() == WL_CONNECTED) {
        configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT, NTP_SERVER);
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5000)) {
            ntpSynced = true;
            Serial.print("[NTP] OK: ");
            Serial.println(getTimeString());
        } else {
            Serial.println("[NTP] FAILED");
        }
    }
}

String getTimeString() {
    struct tm timeinfo;
    if (ntpSynced && getLocalTime(&timeinfo, 100)) {
        char buf[20];
        strftime(buf, sizeof(buf), "%H:%M:%S %d/%m", &timeinfo);
        return String(buf);
    }
    return "";
}
