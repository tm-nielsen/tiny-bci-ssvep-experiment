# include "lsl/lsl_helpers.h"
# include "lsl/lsl_constants.h"

lsl_outlet openIrregularRateLslOutlet(
    const char *streamName, const char *streamType,
    int32_t channelCount,
    lsl_channel_format_t channelFormat,
    const char* sourceId
)
{
    lsl_streaminfo outletInfo = lsl_create_streaminfo(
        streamName, streamType, channelCount,
        LSL_IRREGULAR_RATE, channelFormat, sourceId
    );
    if (outletInfo == NULL) exit(EXIT_FAILURE);

    lsl_outlet outlet = lsl_create_outlet(outletInfo, 0, 360);
    lsl_destroy_streaminfo(outletInfo);

    if (outlet == NULL)
    {
        fprintf(stderr, "Failed to open LSL outlet ");
        fprintf(stderr, "'%s'\n", streamName);
        exit(EXIT_SUCCESS);
    }
    return outlet;
}

void pushLslSample(lsl_outlet outlet, void *sample)
{
    if (outlet == NULL)
    {
        fprintf(stderr, "Error: Can't push to a null outlet\n");
        return;
    }

    int32_t pushError = lsl_push_sample_v(outlet, sample);
    if (pushError != lsl_no_error)
    {
        printf("Error pushing trigger value to LSL stream\n");
        exit(EXIT_SUCCESS);
    }
}

void closeLslOutlet(lsl_outlet *outlet)
{
    if (*outlet == NULL) return;

    lsl_destroy_outlet(*outlet);
    *outlet = NULL;
}

// ---

lsl_inlet connectLslInlet(const char* predicate)
{
    lsl_streaminfo scanResults[2];

    int resultCount = lsl_resolve_bypred
    (
        scanResults, 2, predicate,
        1, LSL_SCAN_TIMEOUT
    );

    if (resultCount < 1)
    {
        fprintf(stderr, "Failed to locate LSL Source ");
        fprintf(stderr, "matching '%s'\n", predicate);
        exit(EXIT_SUCCESS);
    }
    else if (resultCount > 1)
    {
        fprintf(stderr, "Cannot choose between 2 or more ");
        fprintf(stderr, "LSL streams matching '%s'\n", predicate);
        exit(EXIT_SUCCESS);
    }

    lsl_streaminfo targetStream = scanResults[0];
    lsl_inlet inlet = lsl_create_inlet(targetStream, 360, LSL_NO_PREFERENCE, 1);

    for (int i = 0; i < resultCount; i++)
    {
        lsl_destroy_streaminfo(scanResults[i]);
    }

    if (inlet == NULL)
    {
        fprintf(stderr, "Failed to create LSL inlet\n");
        exit(EXIT_SUCCESS);
    }

    int32_t openError = 0;
    lsl_open_stream(inlet, LSL_CONNECT_TIMEOUT, &openError);

    if (openError != lsl_no_error)
    {
        lsl_destroy_inlet(inlet);
        fprintf(stderr, "Failed to connect to LSL stream\n");
        exit(EXIT_SUCCESS);
    }

    return inlet;
}

void closeLslInlet(lsl_inlet *inlet)
{
    if (*inlet == NULL) return;

    lsl_close_stream(*inlet);
    lsl_destroy_inlet(*inlet);
    *inlet = NULL;
}