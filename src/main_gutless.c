# include "presentation.h"
# include "trial_conductor.h"
# include "microsecond_timer.h"

# include "triggers.h"
# include "lsl/trigger_stream.h"
# include "lsl/inference_stream.h"

# include "program_constants.h"


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
void onAllTrialsCompleted()
{
    allTrialsCompleted = true;
    clearPresentationTarget();
}

// ---

void cleanUp()
{
    closeLslTriggerOutlet();
    closeLslInferenceSource();
    stopPresentation();
}

void closeIfPromptedTo()
{
    if (WindowShouldClose())
    {
        cleanUp();
        exit(EXIT_SUCCESS);
    }
}

// ---

int main(void)
{
    initializeTrialConductor(
        N_FREQS, TRIAL_COUNT,
        STIMULUS_DURATION, BREAK_DURATION
    );
    setTrialStartCallback(onTrialStart);
    setTrialEndCallback(onTrialEnd);
    setAllTrialsCompletedCallback(onAllTrialsCompleted);

    initializePresentation(FREQUENCIES, N_FREQS);
    setPresentationTarget(0);
    disableTextureStimulus();

    openLslTriggerOutlet();
    while (!doesLslTriggerOutletHaveConsumers())
    {
        drawMessageScreen("Searching for BCI Engine...");
        closeIfPromptedTo();
    }

    initializeLslInferenceSource();

    MicrosecondTimer connectionAttemptTimer = createMicrosecondTimer(CONNECTION_ATTEMPT_INTERVAL);
    resetMicrosecondTimer(&connectionAttemptTimer);

    while(!isLslInferenceSourceConnected())
    {
        while (!checkMicrosecondTimer(&connectionAttemptTimer))
        {
            drawMessageScreen("Waiting For BCI Engine...");
            closeIfPromptedTo();
        }

        tryConnectLslInferenceSource();
    }

    while (!IsKeyPressed(KEY_SPACE))
    {
        drawMessageScreen("Press Spacebar to Start");
        closeIfPromptedTo();
    }

    while (!WindowShouldClose()) {
        if (allTrialsCompleted)
        {
            drawMessageScreen("Experiment Complete");
            continue;
        }

        updateTrialConductor();

        TinyBCIInference inference;
        uint64_t timestamp;
        if (pollLslInferenceSource(&inference, &timestamp))
        {
            printf("%" PRIu64 " | Output received: %d (%.0f%% confidence) [", timestamp,
                inference.predictedLabel, inference.confidence * 100
            );
            for (int i = 0; i < N_FREQS; i++) printf(" %.2f", inference.confidences[i]);
            printf(" ]\n");

            if (inference.confidence > SELECTION_DISPLAY_THRESHOLD)
            {
                displaySelection(inference.predictedLabel);
            }
        }
        drawStimulusScreen();
    }

    cleanUp();
    return EXIT_SUCCESS;
}