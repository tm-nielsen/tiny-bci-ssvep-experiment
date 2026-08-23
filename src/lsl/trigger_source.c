# include "lsl/trigger_stream.h"
# include "lsl/helpers.h"

static LSLDataSource dataSource;

void initializeLslTriggerSource(TriggerCallback callback)
{
    dataSource = createLSLDataSource(TRIGGER_STREAM_PREDICATE);
    setLSLDataSourceCallback(&dataSource, (void (*)(void*))callback);
}

void updateLslTriggerSource() { pollLSLDataSource(&dataSource); }
void closeLslTriggerSource() { closeLSLDataSource(&dataSource); }

bool tryConnectLslTriggerSource() { return tryConnectLSLDataSource(&dataSource); }
bool isLslTriggerSourceConnected() { return dataSource.isConnected; }