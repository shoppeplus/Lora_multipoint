#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Slave STM32 — Bridge Vibration Monitor
// ============================================

// --- Slave ID (change per device: 1 or 2) ---
#define SLAVE_ID 2

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
