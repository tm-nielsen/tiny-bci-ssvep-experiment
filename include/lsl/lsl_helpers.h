# pragma once
# ifndef LSL_HELPERS
# define LSL_HELPERS

# include "lsl_c.h"

lsl_outlet openIrregularRateLslOutlet(
    const char *streamName, const char *streamType,
    int32_t channelCount,
    lsl_channel_format_t channelFormat,
    const char* sourceId
);
void pushLslSample(lsl_outlet outlet, void *sample);
void closeLslOutlet(lsl_outlet *outlet);

lsl_inlet connectLslInlet(const char* predicate);
void closeLslInlet(lsl_inlet *inlet);

# endif