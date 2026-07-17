# pragma once
# include "data/trigger_source.h"

# define TRIAL_DURATION_MS 10000u // 10 seconds
# define TRIGGER_INTERVAL_MS 14000u;

void initializeTrialConductor(uint16_t, float, float, void (*)(uint16_t), void (*)(uint16_t));
void updateTrialConductor();