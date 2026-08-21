# include "lsl/lsl_trigger_stream.h"
# include "lsl/lsl_helpers.h"
# include "lsl_c.h"

static lsl_outlet outlet = NULL;

void openLslTriggerOutlet(const char *streamName)
{
    outlet = openIrregularRateLslOutlet(
        streamName, TRIGGER_STREAM_TYPE,
        1, cft_int16, TRIGGER_STREAM_SOURCE_ID
    );
}

void pushLslTrigger(uint16_t value)
{
    if (outlet == NULL)
    {
        printf("Attempting to start LSL trigger stream\n");
        openLslTriggerOutlet(TRIGGER_STREAM_NAME_DEFAULT);
    }

    int16_t sample[1] = {value};
    int32_t pushError = lsl_push_sample_s(outlet, sample);

    if (pushError != lsl_no_error)
    {
        printf("Error pushing trigger value to LSL stream\n");
        exit(EXIT_SUCCESS);
    }
}

void closeLslTriggerOutlet() {
    closeLslOutlet(&outlet);
}