# include "pipeline.h"
# include "data/trigger_source.h"
# include "data/synthetic_eeg_source.h"

# include "presentation.h"


int main(int argc, char *argv[])
{
    const float frequencies[N_FREQS] = {7.0f, 8.0f, 9.0f, 11.0f, 7.5f, 8.5f};
    uint16_t target = 0;

    if (initializeTinyBCIPipeline(frequencies)) return EXIT_FAILURE;

    // TODO: wait for user input to "start"

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("Tiny BCI Pipeline Running.\n");

    initializePresentation(frequencies, N_FREQS);
    setPresentationTarget(target);

    resetSyntheticEEGSource();
    resetTriggerSource(target + 1);

    while (!WindowShouldClose())
    {
        updateSyntheticEEGSource();
        updateTriggerSource();

        if (updateTinyBCIPipeline()) break;

        uint16_t inferenceLabel;
        if (tryGetTinyBCIInference(&inferenceLabel))
        {
            printf("Output received: %d\n", inferenceLabel);
            displaySelection(inferenceLabel);
        }

        if (IsKeyPressed(KEY_TAB))
        {
            target = (target + 1) % N_FREQS;
            setPresentationTarget(target);
            setTriggerValue(target + 1);
        }

        updatePresentation();
    }

    stopTinyBCIPipeline();
    stopPresentation();

    return EXIT_SUCCESS;
}