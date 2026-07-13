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
    const float trialDuration = 3.0f;
    const float breakDuration = 1.0f;

    if (initializeTinyBCIPipeline(frequencies)) return EXIT_FAILURE;

    initializePresentation(frequencies, N_FREQS);
    setPresentationTarget(0);

    while (!IsKeyPressed(KEY_SPACE))
    {
        drawEntryScreen();

        if (WindowShouldClose())
        {
            stopTinyBCIPipeline();
            stopPresentation();
            return EXIT_SUCCESS;
        }
    }

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("Tiny BCI Pipeline Running.\n");

    initializeTrialConductor(N_FREQS, trialDuration, breakDuration, onTrialStart, onTrialEnd);
    resetSyntheticEEGSource();

    while (!WindowShouldClose())
    {
        updateSyntheticEEGSource();
        updateTrialConductor();

        if (updateTinyBCIPipeline()) break;

        uint16_t inferenceLabel;
        if (tryGetTinyBCIInference(&inferenceLabel))
        {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printf("%llu | Output received: %d\n", timestamp, inferenceLabel);
            displaySelection(inferenceLabel);
        }

        drawStimulusScreen();
    }

    stopTinyBCIPipeline();
    stopPresentation();

    return EXIT_SUCCESS;
}