#include "pipeline.h"
# include "lsl/trigger_stream.h"
# include "lsl/helpers.h"

static LSLDataSource dataSource;
static TriggerCallback triggerCallback = NULL;

static void passTrigger(void *sampleBuffer)
{
    int16_t sample = *(int16_t*)(dataSource.sampleBuffer);
    double ageSeconds = lsl_local_clock() - dataSource.lastTimestamp;
    if (ageSeconds < 0) ageSeconds = 0;
    uint64_t timestamp = getTimestamp() - (uint64_t)(ageSeconds * 1000000.0);
    triggerCallback((uint16_t)sample, timestamp);
}

void initializeLslTriggerSource(TriggerCallback callback)
{
    triggerCallback = callback;

    dataSource = createLSLDataSource(TRIGGER_STREAM_PREDICATE);
    dataSource.postprocessingFlags = proc_clocksync;
    if (dataSource.inlet != NULL)
    {
        lsl_set_postprocessing(dataSource.inlet, proc_clocksync);
    }
    setLSLDataSourceCallback(&dataSource, passTrigger);
}

void updateLslTriggerSource(void) { pollLSLDataSource(&dataSource); }
void closeLslTriggerSource(void) { closeLSLDataSource(&dataSource); }

bool tryConnectLslTriggerSource(void) { return tryConnectLSLDataSource(&dataSource); }
bool isLslTriggerSourceConnected(void) { return dataSource.isConnected; }