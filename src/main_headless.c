# include "pipeline.h"
# include "microsecond_timer.h"
# include "inference_logger.h"

# include "triggers.h"
# include "lsl/inference_stream.h"
# include "lsl/trigger_stream.h"

# include "program_constants.h"

# define SYNTHETIC_EEG_CHANNEL_COUNT 8
# define SYNTHETIC_EEG_SAMPLE_RATE 250

# include "data/synthetic_eeg_source.h"
void initializeEEGSource()
{
    initializeSyntheticEEGSource(
        SYNTHETIC_EEG_CHANNEL_COUNT,
        SYNTHETIC_EEG_SAMPLE_RATE
    );
}
void updateEEGSource() { updateSyntheticEEGSource(); }
void cleanUpEEGSource() { cleanUpSyntheticEEGSource(); }

uint8_t getChannelCount() { return SYNTHETIC_EEG_CHANNEL_COUNT; }
uint32_t getSampleRate() { return SYNTHETIC_EEG_SAMPLE_RATE; }

static uint16_t currentTargetLabel = 0;
static void onTriggerReceived(uint16_t code)
{
    pushTrigger(code);
    if (code == TRIAL_END_CODE) return; // nothing to do at trial boundaries here
    currentTargetLabel = code - 1; // matches presenter's pushTrigger(target + 1)
}

int main(void)
{
    initializeEEGSource();
    initializeLslTriggerSource(&onTriggerReceived);

    printf("Searching for Presenter App...\n"); 
    MicrosecondTimer connectionAttemptTimer = createMicrosecondTimer(CONNECTION_ATTEMPT_INTERVAL);
    resetMicrosecondTimer(&connectionAttemptTimer);

    while (!isLslTriggerSourceConnected())
    {
        while (!checkMicrosecondTimer(&connectionAttemptTimer))
        {
            updateEEGSource();
            updateTinyBCIPipeline();
        }

        tryConnectLslTriggerSource();
    }
    printf("\nConnected to presenter app\n");

    uint8_t channelCount = getChannelCount();
    uint32_t sampleRate = getSampleRate();
    if (initializeTinyBCIPipeline(FREQUENCIES, channelCount, sampleRate)) return EXIT_FAILURE;
    initializeInferenceLogger();

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("---\nTiny BCI Pipeline Running\n\n");
    printf("Waiting for filter to settle...\n");

    MicrosecondTimer stabilizationTimer = createMicrosecondTimer(FILTER_STABILIZATION_DELAY);
    resetMicrosecondTimer(&stabilizationTimer);
    while (!checkMicrosecondTimer(&stabilizationTimer)) {
        updateEEGSource();
        updateTinyBCIPipeline();
    }
    printf("Filter settled.\n");

    openLslInferenceOutlet();

    while (true)
    {
        updateEEGSource();
        updateLslTriggerSource();

        if (updateTinyBCIPipeline()) break;

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference)) {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printf("%" PRIu64 " | Output received: %d (%.0f%% confidence) [", timestamp,
                inference.predictedLabel, inference.confidence * 100
            );
            for (int i = 0; i < N_FREQS; i++) printf(" %.2f", inference.confidences[i]);
            printf(" ]\n");
            logInference(inference, timestamp, currentTargetLabel);
            pushLslInference(&inference, timestamp, currentTargetLabel);
        }
    }

    cleanUpEEGSource();
    cleanUpTinyBCIPipeline();
    closeLslTriggerSource();
    closeLslInferenceOutlet();
    closeInferenceLogger();
    return EXIT_SUCCESS;
}