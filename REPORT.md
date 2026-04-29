# Bridge Vibration Monitor

## Technical Review Report

Date: 2026-04-28
Workspace: `D:/TIm`

## 1. Executive Summary

This project is a real-time bridge vibration monitoring system built around one ESP32 master and STM32F103 slave nodes with MPU6050 sensors and LoRa radios. The system collects acceleration data, performs FFT-based vibration analysis on the slave side, sends summarized features over LoRa, and exposes status on a web dashboard served by the master.

From a system design perspective, the project is well structured for a prototype or thesis-scale deployment. The codebase has a clear separation of concerns, the slave firmware follows a low-latency respond-first model, and the master firmware combines polling, baseline learning, anomaly detection, and a dashboard in a coherent way.

The main concern is not feature completeness. It is data reliability and state consistency. Several defects in the communication and runtime model can cause stale data acceptance, inconsistent dashboard output, and invalid sensor samples being treated as real measurements. Those issues reduce trust in anomaly detection even though the FFT and overall pipeline are reasonably organized.

## 2. Project Scope and Objective

The project objective is to monitor structural vibration behavior and detect abnormal conditions by:

- sampling accelerometer data on each slave node
- extracting dominant frequency and vibration metrics
- transmitting compact features to a central master over LoRa
- learning a normal baseline over multiple cycles
- raising warnings or critical alerts when live data deviates from baseline
- providing operators with a dashboard over WiFi

Based on the repository layout and `README.md`, the intended deployment is:

- `1` ESP32 master with SX1278 LoRa
- `2` STM32F103 slave nodes with MPU6050 and SX1278 LoRa
- local access through WiFi AP at `192.168.4.1`

## 3. Repository Structure

The repository is divided into two firmware targets:

- `master_esp32/`
- `slave1_stm32/`

The master side contains:

- `src/main.cpp`: system initialization and main control loop
- `src/lora_comm.cpp`: request/reply handling and recalibration workflow
- `src/baseline.cpp`: baseline persistence and anomaly detection
- `src/web_server.cpp`: dashboard API
- `src/wifi_manager.cpp`: AP, STA, and NTP setup

The slave side contains:

- `src/main.cpp`: startup, warm-up, and idle measurement flow
- `src/lora_handler.cpp`: request and calibration command handling
- `src/measurement.cpp`: sampling, smoothing, and response formatting
- `src/fft_analysis.cpp`: FFT and peak detection
- `src/calibration.cpp`: automatic gravity-axis detection
- `src/mpu6050.cpp`: low-level accelerometer access

This split is logical and makes the codebase easy to read compared with many embedded student projects that mix communication, signal processing, and application state in the same file.

## 4. System Architecture

### 4.1 Master Node

The master is responsible for:

- initializing LoRa and network services
- polling each slave in sequence
- parsing feature packets from slaves
- tracking online and offline status
- learning and storing baseline statistics
- computing anomaly scores on the master side
- serving a dashboard over HTTP

The main loop in `master_esp32/src/main.cpp` is straightforward:

1. handle recalibration if requested
2. poll each slave via LoRa
3. update baseline for online nodes
4. enable anomaly detection after all baselines are ready
5. compute final alert state
6. print status and wait until next cycle

### 4.2 Slave Node

Each slave is responsible for:

- reading raw acceleration values from MPU6050
- auto-detecting orientation using gravity
- collecting `256` samples at `1 kHz`
- running FFT and extracting top spectral peaks
- computing RMS, peak, and crest factor
- caching the latest result
- replying to master polls with cached data
- measuring new data immediately after each reply

The respond-first design in `slave1_stm32/src/lora_handler.cpp` is a good fit for this hardware. It reduces reply latency and keeps the master polling cycle predictable.

## 5. Data Processing Pipeline

### 5.1 Sampling

The slave collects `256` samples with a target rate of `1000 Hz` in `measurement.cpp`. Three axes are recorded when multi-axis mode is enabled. The logical vertical axis is selected dynamically through calibration.

### 5.2 Orientation Calibration

At startup, the slave averages `100` accelerometer readings to determine which physical axis aligns with gravity. That axis becomes logical `Z`, and the other two axes become logical `X` and `Y`.

This is a practical feature because it reduces installation sensitivity in field deployments.

### 5.3 FFT and Feature Extraction

The processing sequence implemented in `fft_analysis.cpp` is:

1. remove DC component
2. compute RMS and peak on the AC signal
3. compute crest factor for the vertical axis
4. apply Hann window
5. run in-place radix-2 FFT
6. compute magnitude spectrum
7. extract dominant peaks above a noise threshold

This is technically reasonable for a resource-constrained embedded platform. The output is compact and well suited for LoRa bandwidth limits.

### 5.4 Smoothing

The slave applies exponential moving average smoothing before transmitting data. This reduces noise and keeps the dashboard more stable, but it also means the master is not working with raw event-level measurements.

That tradeoff is acceptable for monitoring, but it should be documented as it affects alert sensitivity and response latency.

## 6. Communication Model

The LoRa protocol described by the project is simple and human-readable:

- `REQ:<id>:<seq>`
- `DATA:<id>,<seq>,...`
- `CALIB:<id>`
- `ACK:<id>,CALIB,<axis>`

The choice to send summarized features instead of raw samples is appropriate. LoRa is a low-bandwidth link, and shipping FFT results is a sensible architectural boundary.

However, the protocol implementation currently has reliability gaps that materially affect data correctness. Those are discussed in the review findings section.

## 7. Dashboard and Runtime Visibility

The ESP32 dashboard provides:

- online and offline node status
- vertical and horizontal vibration metrics
- packet success counts
- baseline learning progress
- alert state
- recalibration and reset-baseline actions

Operationally, this is one of the stronger parts of the project. It makes the system observable enough for demos, local field tests, and debugging.

The main weakness is how the dashboard data is assembled. JSON is built by string concatenation directly from live global state, which creates concurrency and data integrity risk.

## 8. Strengths

- The codebase is modular and relatively easy to navigate.
- The embedded architecture is coherent end to end.
- The slave-side processing boundary is sensible for LoRa constraints.
- Orientation auto-detection improves real-world usability.
- Baseline persistence through NVS is a practical operational feature.
- The dashboard gives meaningful runtime visibility instead of only serial logs.
- Feature selection is appropriate for vibration monitoring: dominant frequency, RMS, peak, and crest factor.

## 9. Review Findings

### Finding 1: Stale LoRa replies can be accepted as fresh data

Location: `master_esp32/src/lora_comm.cpp`

Severity: `P1`

The slave echoes the sequence number, but the master does not validate that the response sequence matches the request it just sent. A delayed packet from an earlier cycle or retry can therefore be accepted as the newest measurement.

Impact:

- marks a node online using stale data
- pollutes baseline learning
- contaminates anomaly detection
- hides timing and communication issues that should have been treated as packet loss

Why this matters:

The system assumes one request produces one logically current reply. Once that assumption fails, the monitoring result is no longer trustworthy even if the radio link itself appears healthy.

### Finding 2: Web handlers read mutable shared state without synchronization

Location: `master_esp32/src/web_server.cpp`

Severity: `P1`

The dashboard API builds JSON directly from global objects that are also being modified by the main loop and recalibration path. This includes mutable `String` members such as `alertReasons` and `calibResult`.

Impact:

- inconsistent API snapshots
- partially updated fields in dashboard responses
- potential heap instability due to unsynchronized `String` access on asynchronous callbacks

Why this matters:

The dashboard is part of the operational interface. If it races with control logic, the system can show misleading status during the exact situations where operators need accurate information.

### Finding 3: Offline nodes retain stale alert state

Location: `master_esp32/src/main.cpp` and `master_esp32/src/lora_comm.cpp`

Severity: `P2`

When a slave times out, the code clears only the `online` flag. Alert fields are not reset, and anomaly computation is skipped for offline nodes. That leaves the previous alert state and reason text in memory.

Impact:

- dashboard can continue to show warning or critical status after disconnect
- operators cannot distinguish current faults from old state
- baseline reset does not fully return the system to a clean monitoring state

Why this matters:

State cleanup is part of correctness. A monitoring system must not show stale alarms once the underlying source is no longer active.

### Finding 4: MPU6050 reads ignore bus errors and short reads

Location: `slave1_stm32/src/mpu6050.cpp`

Severity: `P2`

The accelerometer read path does not validate I2C transaction success, does not check how many bytes were received, and assumes all `6` bytes are always available.

Impact:

- invalid sensor data can enter the signal pipeline
- auto-calibration may choose the wrong gravity axis
- FFT output can be distorted by corrupt samples
- false warnings and false critical alerts become more likely

Why this matters:

The entire analytics chain depends on sensor reads being trustworthy. Without basic bus-level validation, the project has no clean boundary between hardware faults and physical vibration events.

## 10. Risk Assessment

Overall project risk is moderate to high if the firmware is used for real monitoring decisions without hardening.

The largest risks are:

- stale or invalid measurements being treated as current
- dashboard state diverging from control state
- weak fault tolerance at the sensor interface

The current version is adequate for:

- prototype demonstration
- classroom or thesis presentation
- controlled bench testing
- small pilot with manual supervision

The current version is not yet adequate for:

- unattended long-duration deployment
- alarm-driven operational use
- situations where alert accuracy must be defendable

## 11. Recommendations

### 11.1 Immediate Priority

- validate response `seq` on the master before accepting a `DATA` packet
- clear alert-related fields when a slave goes offline
- return sensor read status from `readAccelXYZ()` and discard invalid samples
- stop building dashboard JSON from live mutable state without a protected snapshot

### 11.2 Short-Term Hardening

- replace ad hoc JSON string concatenation with safer serialization
- add time-based freshness checks in addition to sequence checks
- add counters for parse failures, CRC mismatches, and sensor read failures
- record recalibration success and failure per node in a structured way

### 11.3 Medium-Term Improvements

- add unit tests for packet parsing and anomaly calculations
- add integration tests for timeout, retry, and delayed reply scenarios
- separate transport state from presentation state on the master
- consider fixed-size buffers instead of dynamic `String` in hot paths

## 12. Suggested Engineering Roadmap

Phase 1:

- fix the four review findings
- verify stable behavior on bench with forced packet delays and forced I2C faults

Phase 2:

- add structured telemetry for failures
- improve dashboard serialization and state snapshotting
- document timing assumptions and alert semantics

Phase 3:

- test with multiple hours of continuous operation
- characterize false positive and false negative behavior
- review memory stability under sustained dashboard polling

## 13. Final Assessment

This project has a solid prototype architecture and shows good embedded systems thinking, especially in how sensing, communication, and operator visibility are connected. The strongest parts are the modular structure, the slave processing model, and the overall feature set for a bridge vibration monitoring demo.

The main gap is engineering robustness. The current firmware does useful work, but several defects undermine confidence in the truthfulness of the data being displayed and evaluated. That is the exact area a monitoring project cannot afford to be weak in.

With targeted fixes to communication validation, shared-state handling, and sensor read reliability, this project can move from a capable prototype to a more defensible technical system.

## 14. Report Limitations

This report is based on static code review of the repository in `D:/TIm`.

The following activities were not completed in this environment:

- firmware build with PlatformIO
- hardware-in-the-loop testing
- timing validation on real LoRa links
- sensor fault injection

Those steps are still required before drawing conclusions about runtime stability or field readiness.
