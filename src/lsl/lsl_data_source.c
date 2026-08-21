# include "lsl/lsl_helpers.h"
# include "lsl/lsl_constants.h"

LSLDataSource createAndConnectLSLDataSource(const char* streamResolutionPredicate)
{
    lsl_inlet inlet = connectLslInlet(streamResolutionPredicate);

    int32_t infoError = 0;
    lsl_streaminfo inletInfo = lsl_get_fullinfo(inlet, LSL_SCAN_TIMEOUT, &infoError);

    if (infoError != lsl_no_error)
    {
        fprintf(stderr, "Failed to get LSL stream info");
        lsl_destroy_inlet(inlet);
        exit(EXIT_SUCCESS);
    }

    int32_t bufferLength = lsl_get_sample_bytes(inletInfo);
    lsl_destroy_streaminfo(inletInfo);

    return (LSLDataSource)
    {
        .inlet = inlet,
        .sampleBuffer = malloc(bufferLength),
        .sampleBufferLength = bufferLength,
        .bufferMemoryAllocated = true
    };
}

bool pollLSLDataSource(LSLDataSource *source)
{
    if (source == NULL)
    {
        fprintf(stderr, "Error: Attempted to poll a null data source\n");
        return false;
    }
    if (source->inlet == NULL)
    {
        fprintf(stderr,
            "Error: Attempted to poll a data source "
            "without a valid inlet\n"
        );
        return false;
    }

    int32_t pullError = 0;
    double lslTimestamp = lsl_pull_sample_v(
        source->inlet, source->sampleBuffer,
        source->sampleBufferLength, 0.0, &pullError
    );

    if (pullError != lsl_no_error)
    {
        closeLSLDataSource(source);
        printf("Pull error %u in LSL data source\n", pullError);
        exit(EXIT_SUCCESS);
    }

    return lslTimestamp > 0.0;
}

void closeLSLDataSource(LSLDataSource *source)
{
    if (source == NULL) return;
    if (source->bufferMemoryAllocated)
    {
        free(source->sampleBuffer);
        source->bufferMemoryAllocated = false;
    }
    closeLslInlet(&(source->inlet));
}