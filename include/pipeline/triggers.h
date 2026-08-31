# pragma once
# ifndef PIPELINE_TRIGGERS
# define PIPELINE_TRIGGERS

# define TRIAL_END_CODE 10u

uint64_t pushTrigger(uint16_t value);
uint64_t pushTrialEndCode(void);

# endif