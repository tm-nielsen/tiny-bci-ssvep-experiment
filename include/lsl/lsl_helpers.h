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

typedef struct {
    lsl_inlet inlet;
    void *sampleBuffer;
    int32_t sampleBufferLength;
    bool bufferMemoryAllocated;
} LSLDataSource;

LSLDataSource createAndConnectLSLDataSource(const char* streamResolutionPredicate);
bool pollLSLDataSource(LSLDataSource *source);
void closeLSLDataSource(LSLDataSource *source);

# endif