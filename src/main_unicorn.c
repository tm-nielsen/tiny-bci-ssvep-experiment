# include <inttypes.h>
# include <unistd.h>

# include "inference_logger.h"
# include "pipeline.h"
# include "presentation.h"
# include "trial_conductor.h"
# include "microsecond_timer.h"
# include "data/lsl_trigger_outlet.h"
# include "data/trigger_source.h"
# include "data/unicorn_eeg_source.h"

# define PORT "/dev/cu.UN-20230805"

void initializeEEGSource() { initializeUnicornEEGSource(PORT); } // { connectLslEEGSource(); }
void updateEEGSource() { updateUnicornEEGSource(); } //{ updateLslEEGSource(); }
void cleanUpEEGSource() { closeUnicornEEGSource(); } // { disconnectLslEEGSource(); }

uint8_t getChannelCount() { return getUnicornEEGSourceChannelCount(); }
uint32_t getSampleRate() { return getUnicornEEGSourceSampleRate(); }


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


int main(int argc, char *argv[])
{
    const float frequencies[N_FREQS] = {9.0f, 7.5f, 8.0f, 7.0f, 11.0f, 8.57f};

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
    resetUnicornEEGSource();
    setRuntimeConnectionStatus(true);

    openLslTriggerOutlet("tBCI_Experiment_Triggers");

    uint8_t channelCount = getChannelCount();
    uint32_t sampleRate = getSampleRate();
    if (initializeTinyBCIPipeline(frequencies, channelCount, sampleRate)) return EXIT_FAILURE;

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("---\nTiny BCI Pipeline Running.\n\n");

    MicrosecondTimer stabilizationTimer = createMicrosecondTimer(filterStabilizationDelay);
    resetMicrosecondTimer(&stabilizationTimer);
    while (!checkMicrosecondTimer(&stabilizationTimer))
    {
        drawMessageScreen("Awaiting Filter Stabilization...");
        updateEEGSource();
        updateTinyBCIPipeline();

        if (WindowShouldClose())
        {
            cleanUpEEGSource();
            closeLslTriggerOutlet();
            stopPresentation();
            return EXIT_SUCCESS;
        }
    }
    printf("Filter settled.\n");

    while (!IsKeyPressed(KEY_SPACE))
    {
        drawMessageScreen("Press Spacebar to Start");
        updateEEGSource();

        if (WindowShouldClose())
        {
            cleanUpEEGSource();
            closeLslTriggerOutlet();
            stopPresentation();
            return EXIT_SUCCESS;
        }
    }
    initializeInferenceLogger();

    while (!WindowShouldClose())
    {
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

    cleanUpEEGSource();
    cleanUpTinyBCIPipeline();
    closeLslTriggerOutlet();
    closeInferenceLogger();
    stopPresentation();

    return EXIT_SUCCESS;
}