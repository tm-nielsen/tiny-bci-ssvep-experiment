# include "lsl/lsl_eeg_source.h"
# include "lsl/lsl_helpers.h"
# include "lsl/lsl_constants.h"
# include "pipeline.h"
# include "lsl_c.h"

# ifndef USE_LSL_TIMESTAMPS
#   include "microsecond_timer.h"
# endif


static uint32_t sampleRate = 0;
static uint8_t channelCount = 0;

static float* samples = NULL;
static uint32_t sampleIndex = 0;
static bool sampleMemoryAllocated = false;

static lsl_inlet inlet = NULL;

// ---

void connectLslEEGSource()
{
    inlet = connectLslInlet(LSL_EEG_PREDICATE);

    int32_t infoError = 0;
    lsl_streaminfo inletInfo = lsl_get_fullinfo(inlet, LSL_SCAN_TIMEOUT, &infoError);

    if (infoError != lsl_no_error)
    {
        fprintf(stderr, "Failed to get EEG stream info");
        closeLslEEGSource();
        exit(EXIT_SUCCESS);
    }

    channelCount = (uint8_t)lsl_get_channel_count(inletInfo);
    sampleRate = (uint32_t)lsl_get_nominal_srate(inletInfo);

    const char* streamName = lsl_get_name(inletInfo);
    printf("Connected to LSL EEG stream \"%s\"", streamName);
    printf(" with %d channels sampling at %d Hz\n", channelCount, sampleRate);
    printf("---\n");
    lsl_destroy_streaminfo(inletInfo);

    samples = malloc(channelCount * sizeof(float));
    sampleMemoryAllocated = true;
}

// ---

void updateLslEEGSource()
{
    if (!sampleMemoryAllocated)
    {
        printf("Attempting to connect to LSL EEG stream\n");
        connectLslEEGSource();
    }

    int32_t pullError = 0;
    double lslTimestamp = lsl_pull_sample_f(inlet, samples, channelCount, 0.0, &pullError);

    if (pullError != lsl_no_error)
    {
        closeLslEEGSource();
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

void closeLslEEGSource()
{
    if (sampleMemoryAllocated) free(samples);
    closeLslInlet(&inlet);
    sampleMemoryAllocated = false;
}

// ---

uint8_t getLslEEGSourceChannelCount() { return channelCount; }
uint32_t getLslEEGSourceSampleRate() { return sampleRate; }