#include "microsecond_timer.h"
#include "presentation.h"
#include "trial_conductor.h"
#include "data/trigger_source.h"
#include "data/lsl_trigger_outlet.h"
#include "data/lsl_inference_channel.h"

static const float selectionDisplayConfidenceThreshold = 0.5f;
#define INFERENCE_RETRY_INTERVAL_S 5.0f


void onTrialStart(uint16_t target)
{
    pushLslTrigger(target + 1);
    setPresentationTarget(target);
    resumeStimulus();
}

void onTrialEnd(uint16_t nextTarget)
{
    pushLslTrigger(TRIAL_END_CODE);
    setPresentationTarget(nextTarget);
    pauseStimulus();
}

static bool allTrialsCompleted = false;
void onAllTrialsCompleted() { allTrialsCompleted = true; clearPresentationTarget(); }

int main(void)
{
    const float frequencies[N_FREQS] = {9.0f, 7.5f, 8.0f, 7.0f, 11.0f, 8.57f}; // updated by team

    initializeTrialConductor(N_FREQS, 4, 6.0f, 3.0f);
    setTrialStartCallback(onTrialStart);
    setTrialEndCallback(onTrialEnd);
    setAllTrialsCompletedCallback(onAllTrialsCompleted);

    initializePresentation(frequencies, N_FREQS);
    setPresentationTarget(0);
    disableTextureStimulus();

    openLslTriggerOutlet("tBCI_Experiment_Triggers");
    MicrosecondTimer inferenceRetryTimer = createMicrosecondTimer(INFERENCE_RETRY_INTERVAL_S);
    resetMicrosecondTimer(&inferenceRetryTimer);

    while (!IsKeyPressed(KEY_SPACE)) {
        drawMessageScreen("Press Spacebar to Start");

        if (!isLslInferenceConnected() && checkMicrosecondTimer(&inferenceRetryTimer)) {
            if (openLslInferenceInlet(INFERENCE_STREAM_NAME_DEFAULT) == 0) {
                printf("---\nConnected to runtime app\n\n");
            } else {
                printf("---\nFailed to connect to runtime app, retrying...\n\n");
            }
            resetMicrosecondTimer(&inferenceRetryTimer);
        }
        if (WindowShouldClose()) goto shutdown;
    }
    setRuntimeConnectionStatus(isLslInferenceConnected());

    while (!WindowShouldClose()) {
        updateTrialConductor();

        TinyBCIInference inference;
        uint64_t timestamp; uint16_t targetLabel;
        if (pollLslInference(&inference, &timestamp, &targetLabel)) {
            printf("%" PRIu64 " | Output received: %d (%.0f%% confidence) [", timestamp,
                inference.predictedLabel, inference.confidence * 100
            );
            for (int i = 0; i < N_FREQS; i++) printf(" %.2f", inference.confidences[i]);
            printf(" ]\n");
            if (inference.confidence > selectionDisplayConfidenceThreshold) {
                displaySelection(inference.predictedLabel);
            }
        }
        drawStimulusScreen();
    }

    shutdown:
        closeLslTriggerOutlet();
    closeLslInferenceInlet();
    stopPresentation();
    return EXIT_SUCCESS;
}