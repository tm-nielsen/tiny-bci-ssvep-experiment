# pragma once

void initializeTrialConductor(uint16_t, float, float, void (*)(uint16_t), void (*)(uint16_t));
void updateTrialConductor();
uint16_t getTarget();