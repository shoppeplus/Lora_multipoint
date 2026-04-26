#ifndef LORA_COMM_H
#define LORA_COMM_H

#include <Arduino.h>

// Poll slave và parse response
bool pollSlave(uint8_t slaveId);

// Gửi CALIB từng slave (chạy trong loop, an toàn WDT)
void performRecalibrate();

// In kết quả lên Serial
void printResults();

#endif
