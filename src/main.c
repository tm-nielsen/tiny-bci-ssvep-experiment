# include "pipeline.h"
# include "data/synthetic_eeg_source.h"
# include "trial_conductor.h"

# include "presentation.h"


void onTrialStart(uint16_t target)
{
    setPresentationTarget(target);
    resumeStimulus();
}

void onTrialEnd()
{
    clearPresentationTarget();
    pauseStimulus();
}


int main(int argc, char *argv[])
{
    const float frequencies[N_FREQS] = {7.0f, 8.0f, 9.0f, 11.0f, 7.5f, 8.5f};
    const float trialDuration = 3.0f;
    const float breakDuration = 1.0f;

    if (initializeTinyBCIPipeline(frequencies)) return EXIT_FAILURE;

    // TODO: wait for user input to "start"

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("Tiny BCI Pipeline Running.\n");

    initializePresentation(frequencies, N_FREQS);
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
            printf("Output received: %d\n", inferenceLabel);
            displaySelection(inferenceLabel);
        }

        updatePresentation();
    }

    stopTinyBCIPipeline();
    stopPresentation();

    return EXIT_SUCCESS;
}