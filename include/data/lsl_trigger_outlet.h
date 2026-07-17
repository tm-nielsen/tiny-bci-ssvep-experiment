# pragma once
# ifndef LSL_TRIGGER_OUTLET
# define LSL_TRIGGER_OUTLET

# define TRIGGER_STREAM_NAME "tBCI_SSVEP_Triggers"
# define TRIGGER_STREAM_TYPE "Triggers"

void pushLslTrigger(uint16_t);
void closeLslTriggerOutlet();

# endif