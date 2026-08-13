# include "data/lsl_eeg_source.h"
# include "pipeline.h"
# include "lsl_c.h"

# ifndef USE_LSL_TIMESTAMPS
#   include "microsecond_timer.h"
# endif


static uint32_t sampleRate = 0;
static uint8_t channelCount = 0;

static float* samples = NULL;
static uint32_t sampleIndex = 0;

static lsl_inlet inlet = NULL;
static bool isConnected = false;

// ---

void connectLslEEGSource()
{
    lsl_streaminfo scanResult;
    int resultCount = lsl_resolve_byprop(&scanResult, 1, "type", "EEG", 1, LSL_SCAN_TIMEOUT);

    if (resultCount < 1)
    {
        printf("Failed to locate LSL EEG Source\n");
        exit(EXIT_SUCCESS);
    }

    channelCount = (uint8_t)lsl_get_channel_count(scanResult);
    sampleRate = (uint32_t)lsl_get_nominal_srate(scanResult);

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

    samples = malloc(channelCount * sizeof(float));
}

// ---

void updateLslEEGSource()
{
    if (!isConnected)
    {
        printf("Attempting to connect to LSL EEG stream\n");
        connectLslEEGSource();
    }

    int32_t pullError = 0;
    double lslTimestamp = lsl_pull_sample_f(inlet, samples, channelCount, 0.0, &pullError);

    if (pullError != lsl_no_error)
    {
        disconnectLslEEGSource();
        printf("Pull error %u in LSL EEG source\n", pullError);
        exit(EXIT_SUCCESS);
    }

    if (lslTimestamp > 0.0)
    {
# ifdef USE_LSL_TIMESTAMPS
        uint64_t microsecondTimestamp = (uint64_t)(lslTimestamp * 1000000);
# else
        uint64_t microsecondTimestamp = getCurrentMicrosecondTimestamp();
# endif
        in_push_signal(&tbciInputs, samples, microsecondTimestamp, sampleIndex++);
    }
}

void disconnectLslEEGSource()
{
    if (isConnected) free(samples);
    if (inlet != NULL)
    {
        lsl_close_stream(inlet);
        lsl_destroy_inlet(inlet);
        inlet = NULL;
    }
    isConnected = false;
}

// ---

uint8_t getLslEEGSourceChannelCount() { return channelCount; }
uint32_t getLslEEGSourceSampleRate() { return sampleRate; }