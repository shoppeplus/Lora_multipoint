#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Master ESP32 — Bridge Vibration Monitor
// ============================================

// --- LoRa RA-02 (SX1278) — SPI ---
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23

// --- LoRa RF (must match Slave) ---
#define LORA_FREQ          433E6
#define LORA_SF            7
#define LORA_BW            125E3
#define LORA_CR            5
#define LORA_TX_POWER      17

// --- Polling Protocol ---
#define NUM_SLAVES         2
#define POLL_TIMEOUT_MS    600     // Wait for slave response
#define POLL_INTERVAL_MS   200     // Pause between full cycles
#define GUARD_TIME_MS      50      // Guard between polling slaves

// --- Retry ---
#define MAX_RETRIES        1
#define RETRY_DELAY_MS     100

// --- Multi-Frequency ---
#define NUM_TOP_FREQS      3

// --- Baseline & Anomaly Detection ---
#define BASELINE_CYCLES    10      // Learning period (cycles)
#define ANOMALY_SIGMA      3.0f   // Z-score threshold

// --- Alert Levels ---
#define ALERT_NORMAL    0
#define ALERT_WARNING   1
#define ALERT_CRITICAL  2

// --- WiFi ---
#define WIFI_AP_SSID       "BridgeMonitor"
#define WIFI_AP_PASS       "bridge2024"
#define WIFI_STA_SSID      "CUONG THU"
#define WIFI_STA_PASS      "00000000"
#define WIFI_AP_CHANNEL    6
#define WIFI_AP_MAX_CONN   4

// --- NTP ---
#define NTP_SERVER         "pool.ntp.org"
#define NTP_GMT_OFFSET     25200   // UTC+7
#define NTP_DAYLIGHT       0

// --- Serial ---
#define SERIAL_BAUD        115200

#endif
