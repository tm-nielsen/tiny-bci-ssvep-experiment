# include "pipeline.h"
# include "data/synthetic_eeg_source.h"
# include "trial_conductor.h"

# include "presentation.h"
# include "microsecond_timer.h"


void onTrialStart(uint16_t target)
{
    setPresentationTarget(target);
    resumeStimulus();
}

void onTrialEnd(uint16_t nextTarget)
{
    setPresentationTarget(nextTarget);
    pauseStimulus();
}


int main(int argc, char *argv[])
{
    const float frequencies[N_FREQS] = {7.0f, 8.0f, 9.0f, 11.0f, 7.5f, 8.5f};
    const float trialDuration = 6.0f;
    const float breakDuration = 3.0f;
    const float selectionDisplayConfidenceThreshold = 0.5f;

    initializeTrialConductor(N_FREQS, trialDuration, breakDuration, onTrialStart, onTrialEnd);
    initializePresentation(frequencies, N_FREQS);
    setPresentationTarget(0);

    while (!IsKeyPressed(KEY_SPACE))
    {
        drawEntryScreen();

        if (WindowShouldClose())
        {
            stopPresentation();
            return EXIT_SUCCESS;
        }
    }

    if (initializeTinyBCIPipeline(frequencies)) return EXIT_FAILURE;
    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("Tiny BCI Pipeline Running.\n");

    resetSyntheticEEGSource();

    while (!WindowShouldClose())
    {
        updateSyntheticEEGSource();
        updateTrialConductor();

        if (updateTinyBCIPipeline()) break;

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference))
        {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printf("%llu | Output received: %d, %.0f%%\n", timestamp,
                inference.predictedLabel, inference.confidence * 100
            );

            if (inference.confidence > selectionDisplayConfidenceThreshold)
            {
                displaySelection(inference.predictedLabel);
            }
        }

        drawStimulusScreen();
    }

    stopTinyBCIPipeline();
    stopPresentation();

    return EXIT_SUCCESS;
}