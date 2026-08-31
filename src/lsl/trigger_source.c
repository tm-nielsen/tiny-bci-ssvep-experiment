# include "lsl/trigger_stream.h"
# include "lsl/helpers.h"

static LSLDataSource dataSource;
static TriggerCallback triggerCallback = NULL;

static void passTrigger(void *sampleBuffer)
{
    int16_t sample = *(int16_t*)(dataSource.sampleBuffer);
    triggerCallback(sample);
}

void initializeLslTriggerSource(TriggerCallback callback)
{
    triggerCallback = callback;

    dataSource = createLSLDataSource(TRIGGER_STREAM_PREDICATE);
    setLSLDataSourceCallback(&dataSource, passTrigger);
}

void updateLslTriggerSource(void) { pollLSLDataSource(&dataSource); }
void closeLslTriggerSource(void) { closeLSLDataSource(&dataSource); }

bool tryConnectLslTriggerSource(void) { return tryConnectLSLDataSource(&dataSource); }
bool isLslTriggerSourceConnected(void) { return dataSource.isConnected; }