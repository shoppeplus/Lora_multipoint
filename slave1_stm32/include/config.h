#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Slave STM32 - Bridge Vibration Monitor v3.2
// Auto-Orientation + Fast Continuous Monitoring
// ============================================

// --- ID Slave (THAY ĐỔI: 1 hoặc 2) ---
#define SLAVE_ID   2

// --- LoRa RA-02 (SX1278) Pins - SPI1 ---
#define LORA_SS    PA4
#define LORA_RST   PB9
#define LORA_DIO0  PB8
#define LORA_SCK   PA5
#define LORA_MISO  PA6
#define LORA_MOSI  PA7

// --- MPU6050 - I2C1 ---
#define MPU_SDA         PB7
#define MPU_SCL         PB6
#define MPU_ADDR        0x68
#define PWR_MGMT_1      0x6B
#define ACCEL_XOUT_H    0x3B
#define ACCEL_CONFIG     0x1C
#define SMPLRT_DIV       0x19
#define MPU_CONFIG       0x1A

// --- LoRa Parameters (phải trùng với Master) ---
#define LORA_FREQ       433E6
#define LORA_SF         7
#define LORA_BW         125E3
#define LORA_CR         5
#define LORA_TX_POWER   17

// --- FFT Parameters ---
#define SAMPLE_SIZE     256
#define SAMPLE_RATE     1000

// --- Multi-Frequency Analysis ---
#define NUM_TOP_FREQS       3
#define NUM_TOP_FREQS_XY    1
#define MIN_PEAK_DISTANCE   3

// --- Multi-Axis FFT ---
#define ENABLE_MULTI_AXIS   1

// --- Noise Filter ---
#define NOISE_FLOOR_MAG     5.0f
#define EMA_ALPHA           0.3f

// --- Auto-Orientation Calibration ---
#define CALIB_SAMPLES       100   // Số mẫu để xác định trục gravity
#define GRAVITY_THRESHOLD   0.7f  // |mean| > 0.7g → đó là trục gravity

// --- Impact Detection (giá trị AC, sau trừ gravity) ---
#define IMPACT_WARNING_G    0.1f  // Dao động AC > 0.1g → WARNING
#define IMPACT_CRITICAL_G   0.5f  // Dao động AC > 0.5g → CRITICAL
#define CF_WARNING          5.0f
#define CF_CRITICAL         10.0f

// --- Alert Levels ---
#define ALERT_NORMAL    0
#define ALERT_WARNING   1
#define ALERT_CRITICAL  2

// --- Continuous Measurement ---
#define MEASURE_INTERVAL_MS  2000   // Fallback: đo nếu không có poll trong 2s

// --- Serial ---
#define SERIAL_BAUD     115200

#endif // CONFIG_H
