#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Preferences.h>
#include "data_types.h"

extern SlaveData    slaves[NUM_SLAVES];
extern BaselineData baselines[NUM_SLAVES];
extern unsigned long cycleCount;
extern unsigned long globalSeqNum;
extern bool baselineComplete;

extern String apIP;
extern String staIP;
extern bool ntpSynced;

extern Preferences prefs;

// Recalibrate
extern volatile bool needRecalibrate;
extern bool calibDone;
extern int calibAckCount;
extern String calibResult;

// History ring buffer (1 hour, auto-overwrite)
extern HistoryPoint history[NUM_SLAVES][HISTORY_MAX];
extern int historyHead[NUM_SLAVES];
extern int historyCount[NUM_SLAVES];
extern unsigned long lastHistoryTime;

#endif
