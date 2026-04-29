#include "globals.h"

SlaveData    slaves[NUM_SLAVES];
BaselineData baselines[NUM_SLAVES];
unsigned long cycleCount = 0;
unsigned long globalSeqNum = 0;
bool baselineComplete = false;

String apIP = "";
String staIP = "";
bool ntpSynced = false;

Preferences prefs;

volatile bool needRecalibrate = false;
bool calibDone = false;
int calibAckCount = 0;
String calibResult = "";

HistoryPoint history[NUM_SLAVES][HISTORY_MAX];
int historyHead[NUM_SLAVES] = {0};
int historyCount[NUM_SLAVES] = {0};
unsigned long lastHistoryTime = 0;
