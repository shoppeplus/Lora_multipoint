#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include <Arduino.h>

// Handle incoming LoRa packets (REQ polling + CALIB commands)
bool checkAndRespond();

#endif
