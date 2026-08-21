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

static LSLDataSource dataSource;
static uint32_t sampleIndex = 0;

// ---

void connectLslEEGSource()
{
    dataSource = createAndConnectLSLDataSource(EEG_STREAM_PREDICATE);

    int32_t infoError = 0;
    lsl_streaminfo inletInfo = lsl_get_fullinfo(
        dataSource.inlet, LSL_SCAN_TIMEOUT, &infoError
    );

    if (infoError != lsl_no_error)
    {
        fprintf(stderr, "Failed to get EEG stream info");
        closeLslEEGSource();
        exit(EXIT_SUCCESS);
    }

    channelCount = (uint8_t)lsl_get_channel_count(inletInfo);
    sampleRate = (uint32_t)lsl_get_nominal_srate(inletInfo);

    const char* streamName = lsl_get_name(inletInfo);
    printf(
        "Connected to LSL EEG stream '%s' with "
        "%d channels sampling at %d Hz\n"
        , streamName, channelCount, sampleRate
    );
    printf("---\n");
    lsl_destroy_streaminfo(inletInfo);
}

// ---

void updateLslEEGSource()
{
    if (pollLSLDataSource(&dataSource))
    {
# ifdef USE_LSL_TIMESTAMPS
        uint64_t microsecondTimestamp = (uint64_t)(lslTimestamp * 1000000);
# else
        uint64_t microsecondTimestamp = getCurrentMicrosecondTimestamp();
# endif

        in_push_signal(
            &tbciInputs, (float*)dataSource.sampleBuffer,
            microsecondTimestamp, sampleIndex++
        );
    }
}

void closeLslEEGSource()
{
    closeLSLDataSource(&dataSource);
}

// ---

uint8_t getLslEEGSourceChannelCount() { return channelCount; }
uint32_t getLslEEGSourceSampleRate() { return sampleRate; }