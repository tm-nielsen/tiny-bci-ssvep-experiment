# include "lsl/trigger_stream.h"
# include "lsl/helpers.h"

static LSLDataSource dataSource;
static TriggerCallback triggerCallback = NULL;

void connectLslTriggerSource(TriggerCallback callback)
{
    dataSource = createAndConnectLSLDataSource(TRIGGER_STREAM_PREDICATE);
    triggerCallback = callback;
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