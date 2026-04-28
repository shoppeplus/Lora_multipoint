# Bridge Vibration Monitor v3.2

Real-time structural health monitoring system for bridges using vibration analysis.

## Architecture

- **1 Master** (ESP32 + LoRa RA-02) — Collects data from slaves, serves web dashboard
- **2 Slaves** (STM32F103 + MPU6050 + LoRa RA-02) — Measure vibrations, compute FFT, transmit results

## Features

- **Multi-axis FFT analysis** — 256-point Cooley-Tukey FFT at 1kHz sampling rate
- **Auto-orientation** — Automatic gravity axis detection, mount sensor in any direction
- **Always-On architecture** — Slaves respond instantly to polls, measure after transmitting
- **3σ anomaly detection** — Baseline learning with statistical deviation alerts
- **Web dashboard** — Real-time monitoring via WiFi AP (192.168.4.1)
- **LoRa communication** — 433MHz, SF7, 125kHz bandwidth, ~1km range

## Hardware

| Component | Master | Slave (x2) |
|---|---|---|
| MCU | ESP32-WROOM | STM32F103C8T6 |
| Radio | LoRa RA-02 (SX1278) | LoRa RA-02 (SX1278) |
| Sensor | — | MPU6050 (±2g, 1kHz) |

### Pin Configuration

**Master (ESP32):**
```
LoRa: SS=5, RST=14, DIO0=2, SCK=18, MISO=19, MOSI=23
```

**Slave (STM32):**
```
LoRa: SS=PA4, RST=PB9, DIO0=PB8, SCK=PA5, MISO=PA6, MOSI=PA7
MPU6050: SDA=PB7, SCL=PB6, ADDR=0x68
```

## Project Structure

```
├── master_esp32/          # ESP32 Master firmware
│   ├── include/
│   │   ├── config.h       # Pin/RF/WiFi configuration
│   │   ├── data_types.h   # SlaveData, BaselineData structs
│   │   ├── globals.h      # Shared global variables
│   │   ├── dashboard.h    # HTML/CSS/JS web dashboard
│   │   ├── wifi_manager.h # WiFi AP+STA + NTP
│   │   ├── web_server.h   # REST API endpoints
│   │   ├── baseline.h     # Baseline learning + anomaly detection
│   │   └── lora_comm.h    # LoRa polling + recalibrate
│   └── src/
│       ├── main.cpp       # Setup + main loop
│       ├── globals.cpp    # Global variable definitions
│       ├── wifi_manager.cpp
│       ├── web_server.cpp
│       ├── baseline.cpp
│       └── lora_comm.cpp
│
└── slave1_stm32/          # STM32 Slave firmware
    ├── include/
    │   ├── config.h       # Slave ID, pins, FFT parameters
    │   ├── mpu6050.h      # Accelerometer driver
    │   ├── calibration.h  # Auto-orientation calibration
    │   ├── fft_analysis.h # FFT + peak detection
    │   ├── measurement.h  # Sample collection + EMA smoothing
    │   └── lora_handler.h # REQ/CALIB command handler
    └── src/
        ├── main.cpp       # Setup + main loop
        ├── mpu6050.cpp
        ├── calibration.cpp
        ├── fft_analysis.cpp
        ├── measurement.cpp
        └── lora_handler.cpp
```

## Flashing

Change `SLAVE_ID` in `slave1_stm32/include/config.h` before flashing each slave:
```c
#define SLAVE_ID   1    // Set to 1 or 2
```

Build with PlatformIO:
```bash
# Master
cd master_esp32 && pio run --target upload

# Slave
cd slave1_stm32 && pio run --target upload
```

## Dashboard

Connect to WiFi AP **BridgeMonitor** (password: `bridge2024`), open http://192.168.4.1

## LoRa Protocol

```
Master → Slave:  REQ:<id>:<seq>
Slave → Master:  DATA:<id>,<seq>,<f1z>,<f2z>,<f3z>,<rmsZ>,<peakZ>,<cfZ>,<alert>,<f1x>,<rmsX>,<peakX>,<f1y>,<rmsY>,<peakY>
Master → Slave:  CALIB:<id>
Slave → Master:  ACK:<id>,CALIB,<axis>
```
