# include "data/lsl_eeg_source.h"
# include "data/eeg_source.h"
# include "pipeline.h"
# include "lsl_c.h"

static float samples[CHANNEL_COUNT];
static uint32_t sampleIndex = 0;

static lsl_inlet inlet = NULL;
static bool isConnected = false;


bool validateStreamInfo(lsl_streaminfo *info)
{
    int32_t channelCount = lsl_get_channel_count(*info);
    if (channelCount != CHANNEL_COUNT)
    {
        printf("Channel count (%u) mismatch with discovered EEG stream\n", channelCount);
        printf("Must be configured to match in 'eeg_source.h'\n");
        return false;
    }

    int32_t sampleRate = (int32_t)lsl_get_nominal_srate(*info);
    if (sampleRate != SAMPLE_RATE)
    {
        printf("Sample rate (%u) mismatch with discovered EEG stream\n", sampleRate);
        printf("Must be configured to match in 'eeg_source.h'\n");
        return false;
    }
    return true;
}

void connectLslEEGSource()
{
    lsl_streaminfo scanResult;
    int resultCount = lsl_resolve_byprop(&scanResult, 1, "type", "EEG", 1, LSL_SCAN_TIMEOUT);

    if (resultCount < 1)
    {
        printf("Failed to locate LSL EEG Source\n");
        exit(EXIT_SUCCESS);
    }

    if (!validateStreamInfo(&scanResult)) exit(EXIT_SUCCESS);

    inlet = lsl_create_inlet(scanResult, 360, LSL_NO_PREFERENCE, 1);
    lsl_destroy_streaminfo(scanResult);

    if (inlet == NULL)
    {
        printf("Failed to create LSL inlet\n");
        exit(EXIT_SUCCESS);
    }

    int32_t openError = 0;
    lsl_open_stream(inlet, LSL_CONNECT_TIMEOUT, &openError);

    if (openError != lsl_no_error)
    {
        lsl_destroy_inlet(inlet);
        printf("Failed to connect to LSL EEG stream\n");
        exit(EXIT_SUCCESS);
    }
    isConnected = true;
}

void updateLslEEGSource()
{
    if (!isConnected)
    {
        printf("Attempting to connect to LSL EEG stream\n");
        connectLslEEGSource();
    }

    int32_t pullError = 0;
    double lslTimestamp = lsl_pull_sample_f(inlet, samples, CHANNEL_COUNT, 0.0, &pullError);

    if (pullError != lsl_no_error)
    {
        disconnectLslEEGSource();
        printf("Pull error %u in LSL EEG source\n", pullError);
        exit(EXIT_SUCCESS);
    }

    if (lslTimestamp > 0.0)
    {
        uint64_t lslTimestampMicroseconds = (uint64_t)(lslTimestamp * 1000000);
        in_push_signal(&tbciInputs, samples, lslTimestampMicroseconds, sampleIndex++);
    }
}

void disconnectLslEEGSource()
{
    if (inlet != NULL)
    {
        lsl_close_stream(inlet);
        lsl_destroy_inlet(inlet);
        inlet = NULL;
    }
    isConnected = false;
}