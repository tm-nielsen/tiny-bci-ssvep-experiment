# pragma once
# include "storage.h"

# define TRIGGER_INTERVAL_MS 2000u

# define TRIAL_END_CODE 10u
# define TRIAL_DURATION_MS 3000u

struct Trial {
    uint64_t startTime;
    uint64_t endTime;
    struct Trial *next;
};
typedef struct Trial* TrialPointer;

void resetTriggerSource(uint16_t);
void updateTriggerSource();

void setTriggerValue(uint16_t);