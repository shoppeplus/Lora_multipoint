#ifndef BASELINE_H
#define BASELINE_H

void saveBaseline();
void loadBaseline();
void resetBaselineData();
void updateBaseline(int slaveIdx);
int  checkAnomaly(int slaveIdx);

#endif
