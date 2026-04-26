#ifndef LORA_COMM_H
#define LORA_COMM_H

#include <Arduino.h>

bool pollSlave(uint8_t slaveId);
void performRecalibrate();
void printResults();

#endif
