# include "lsl/trigger_stream.h"
# include "lsl/helpers.h"

static LSLDataSource dataSource;
static TriggerCallback triggerCallback = NULL;

bool tryConnectLslTriggerSource(TriggerCallback callback)
{
    triggerCallback = callback;
    return tryConnectLSLDataSource(&dataSource, TRIGGER_STREAM_PREDICATE);
}

void updateLslTriggerSource()
{
    if (pollLSLDataSource(&dataSource))
    {
        int16_t sample = *(int16_t*)(dataSource.sampleBuffer);
        triggerCallback(sample);
    }
}

void closeLslTriggerSource()
{
    closeLSLDataSource(&dataSource);
}