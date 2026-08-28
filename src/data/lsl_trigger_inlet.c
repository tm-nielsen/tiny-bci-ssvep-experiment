# include "data/lsl_trigger_inlet.h"

static lsl_inlet inlet = NULL;
static TriggerCallback triggerCallback = NULL;

int openLslTriggerInlet(const char *streamName, TriggerCallback callback)
{
    triggerCallback = callback;

    lsl_streaminfo resolved[1];
    int resolvedCount = lsl_resolve_byprop(resolved, 1, "name", streamName, 1, 0.2); // was LSL_FOREVER

    if (resolvedCount < 1) {
        printf("resolve failed for '%s', count=%d\n", streamName, resolvedCount); // <-- add here
        return -1;
    }

    inlet = lsl_create_inlet(resolved[0], 360, LSL_NO_PREFERENCE, 1);
    lsl_destroy_streaminfo(resolved[0]);

    if (inlet == NULL) return -1;

    lsl_open_stream(inlet, LSL_FOREVER, NULL);
    return 0;
}

void pollLslTriggerInlet(void)
{
    if (inlet == NULL) return;

    for (;;)
    {
        int16_t sample[1];
        double timestamp = lsl_pull_sample_s(inlet, sample, 1, 0.0, NULL);

        if (timestamp == 0.0) break; // no sample currently available, non-blocking

        if (triggerCallback != NULL)
        {
            triggerCallback((uint16_t)sample[0]);
        }
    }
}

void closeLslTriggerInlet(void)
{
    if (inlet != NULL)
    {
        lsl_destroy_inlet(inlet);
        inlet = NULL;
    }
}

bool isLslTriggerConnected(void)
{
    return inlet != NULL;
}