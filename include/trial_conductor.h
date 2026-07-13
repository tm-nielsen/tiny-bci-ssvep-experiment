# pragma once
# include "data/trigger_source.h"

# define TRIAL_DURATION_MS 6000u
# define TRIGGER_INTERVAL_MS 2000u;

void initializeTrialConductor(uint16_t, float, float, void (*)(uint16_t), void (*)());
void updateTrialConductor();