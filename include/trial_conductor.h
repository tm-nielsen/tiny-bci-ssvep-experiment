# pragma once

void initializeTrialConductor(uint16_t, float, float, void (*)(uint16_t), void (*)(uint16_t));
void updateTrialConductor();
void resetTrialConductorTimers(void);

uint16_t getTarget();