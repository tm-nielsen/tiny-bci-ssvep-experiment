# include "pipeline.h"
# include "triggers.h"
# include "presentation.h"
# include "trial_conductor.h"
# include "microsecond_timer.h"

# include "inference_logger.h"

# include "lsl/lsl_trigger_stream.h"

# include "lsl/lsl_eeg_source.h"
void initializeEEGSource() { connectLslEEGSource(); }
void updateEEGSource() { updateLslEEGSource(); }
void cleanUpEEGSource() { closeLslEEGSource(); }

uint8_t getChannelCount() { return getLslEEGSourceChannelCount(); }
uint32_t getSampleRate() { return getLslEEGSourceSampleRate(); }


static uint16_t currentTargetLabel = 0;
void onTrialStart(uint16_t target)
{
    currentTargetLabel = target;
    pushTrigger(target + 1);
    pushLslTrigger(target + 1);
    setPresentationTarget(target);
    resumeStimulus();
}

void onTrialEnd(uint16_t nextTarget)
{
    pushTrigger(TRIAL_END_CODE);
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
    cleanUpEEGSource();
    cleanUpTinyBCIPipeline();
    closeLslTriggerOutlet();
    closeInferenceLogger();
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

void updatePipelineWithUserPrompt(const char* message)
{
    drawMessageScreen(message);
    updateEEGSource();
    updateTinyBCIPipeline();
    closeIfPromptedTo();
}


int main(int argc, char *argv[])
{
    const float frequencies[N_FREQS] = {7.5f, 8.57f, 10.0f, 12.0f};

    const uint16_t stimulusRounds = 4;

    const float filterStabilizationDelay = 5.0f;
    const float stimulusDuration = 6.0f;
    const float breakDuration = 3.0f;

    const float selectionDisplayConfidenceThreshold = 0.9f;

    initializeTrialConductor(N_FREQS, stimulusRounds, stimulusDuration, breakDuration);
    setTrialStartCallback(onTrialStart);
    setTrialEndCallback(onTrialEnd);
    setAllTrialsCompletedCallback(onAllTrialsCompleted);

    initializePresentation(frequencies, N_FREQS);
    setPresentationTarget(0);
    disableTextureStimulus();

    initializeEEGSource();
    openLslTriggerOutlet("tBCI_Experiment_Triggers");

    uint8_t channelCount = getChannelCount();
    uint32_t sampleRate = getSampleRate();
    if (initializeTinyBCIPipeline(frequencies, channelCount, sampleRate)) return EXIT_FAILURE;
    initializeInferenceLogger();

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("---\nTiny BCI Pipeline Running.\n\n");

    MicrosecondTimer stabilizationTimer = createMicrosecondTimer(filterStabilizationDelay);
    resetMicrosecondTimer(&stabilizationTimer);
    while (!checkMicrosecondTimer(&stabilizationTimer))
    {
        updatePipelineWithUserPrompt("Awaiting Filter Stabilization...");
    }
    printf("Filter settled.\n");

    while (!IsKeyPressed(KEY_SPACE))
    {
        updatePipelineWithUserPrompt("Press Spacebar to Start");
    }

    while (!WindowShouldClose())
    {
        if (allTrialsCompleted)
        {
            drawMessageScreen("Experiment Complete");
            continue;
        }

        updateEEGSource();
        updateTrialConductor();

        if (updateTinyBCIPipeline()) break;

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference))
        {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printf("%" PRIu64 " | Output received: %d (%.0f%% confidence) [", timestamp,
                inference.predictedLabel, inference.confidence * 100
            );
            for (int i = 0; i < N_FREQS; i++) printf(" %.2f", inference.confidences[i]);
            printf(" ]\n");

            logInference(inference, timestamp, currentTargetLabel);

            if (inference.confidence > selectionDisplayConfidenceThreshold)
            {
                displaySelection(inference.predictedLabel);
            }
        }

        drawStimulusScreen();
    }

    cleanUp();
    return EXIT_SUCCESS;
}