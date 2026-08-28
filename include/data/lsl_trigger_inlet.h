#ifndef LSL_TRIGGER_INLET_H
#define LSL_TRIGGER_INLET_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "lsl_c.h"

#include "data/lsl_trigger_outlet.h" // shares TRIGGER_STREAM_TYPE / SOURCE_ID / NAME_DEFAULT

typedef void (*TriggerCallback)(uint16_t value);

int openLslTriggerInlet(const char *streamName, TriggerCallback callback);
void pollLslTriggerInlet(void); // call every runtime loop iteration; pumps callback for any pending samples
void closeLslTriggerInlet(void);
bool isLslTriggerConnected(void);

#endif