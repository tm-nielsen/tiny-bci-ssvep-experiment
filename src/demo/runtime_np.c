#include <inttypes.h>

#include "pipeline.h"
#include "microsecond_timer.h"
#include "inference_logger.h"
#include "data/lsl_eeg_source.h"
#include "data/synthetic_eeg_source.h"    // present in team branch, unused unless swapped in
#include "data/lsl_trigger_inlet.h"       // new: mirror of your existing outlet, inlet side
#include "data/lsl_inference_channel.h"
#include "data/trigger_source.h"
#include "data/neuropawn_eeg_source.h"

static uint16_t currentTargetLabel = 0;
# define TRIGGER_RETRY_INTERVAL_S 5.0f
# define PORT "/dev/tty.usbserial-A5069RR4"

void initializeEEGSource() { connectNeuropawnEEGSource(PORT, NEUROPAWN_DEFAULT_CONFIGURATION); } // { connectLslEEGSource(); }
void updateEEGSource() { updateNeuropawnEEGSource(); } //{ updateLslEEGSource(); }
void cleanUpEEGSource() { disconnectNeuropawnEEGSource(); } // { disconnectLslEEGSource(); }

uint8_t getChannelCount() { return getNeuropawnEEGSourceChannelCount(); }
uint32_t getSampleRate() { return getNeuropawnEEGSourceSampleRate(); }



static void onTriggerReceived(uint16_t code)
{
    pushTrigger(code);
    if (code == TRIAL_END_CODE) return; // nothing to do at trial boundaries here
    currentTargetLabel = code - 1; // matches presenter's pushTrigger(target + 1)
}

int main(void)
{
    const float frequencies[N_FREQS] = {9.0f, 7.5f, 8.0f, 7.0f, 11.0f, 8.57f};
    const int selectedChannelCount = 8;
    const int selectedChannels[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    const float targetSampleRate = 125.0f;

    initializeEEGSource();
    //resetUnicornEEGSource();
    openLslInferenceOutlet("tBCI_Inference_Results");
    openLslTriggerInlet("tBCI_Experiment_Triggers", onTriggerReceived);

    MicrosecondTimer triggerRetryTimer = createMicrosecondTimer(TRIGGER_RETRY_INTERVAL_S);
    resetMicrosecondTimer(&triggerRetryTimer);

    uint8_t channelCount = getChannelCount();
    uint32_t sampleRate = getSampleRate();
    if (initializeTinyBCIPipeline(frequencies, channelCount, selectedChannelCount, selectedChannels, sampleRate, targetSampleRate)) return EXIT_FAILURE;

    if (startTinyBCIPipeline()) return EXIT_FAILURE;

    printf("---\ntinyBCI Runtime running headless\n\n");

    initializeInferenceLogger();

    // filter stabilization delay, no display, just poll EEG + pipeline
    MicrosecondTimer stabilizationTimer = createMicrosecondTimer(5.0f);
    resetMicrosecondTimer(&stabilizationTimer);
    while (!checkMicrosecondTimer(&stabilizationTimer)) {
        updateEEGSource();
        updateTinyBCIPipeline();
        pollLslTriggerInlet(); // pumps onTriggerReceived callback

        if (checkMicrosecondTimer(&triggerRetryTimer)) {
            if (!isLslTriggerConnected())
            {
                if (openLslTriggerInlet("tBCI_Experiment_Triggers", onTriggerReceived) == 0) {
                    printf("---\nConnected to presenter app\n\n");
                } else {
                    printf("---\nFailed to connect to presenter app, retrying...\n\n");
                }
            }
            resetMicrosecondTimer(&triggerRetryTimer);
        }
    }

    for (;;) {
        updateEEGSource();
        pollLslTriggerInlet();

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
    closeLslTriggerInlet();
    closeLslInferenceOutlet();
    closeInferenceLogger();
    return EXIT_SUCCESS;
}