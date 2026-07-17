#include <unistd.h>

# include "pipeline.h"
# include "data/unicorn_eeg_source.h"
# include "trial_conductor.h"

# include "presentation.h"
# include "microsecond_timer.h"

#define PORT "/dev/cu.UN-20230805"

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
    const float frequencies[N_FREQS] = {9.0f, 7.5f, 8.0f, 7.0f, 11.0f, 8.5f}; //15, 10, 12, 8.5
    const float trialDuration = 12.0f;
    const float breakDuration = 3.0f;
    const float selectionDisplayConfidenceThreshold = 0.80f;

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

    initializeUnicornEEGSource(PORT);
    resetUnicornEEGSource();
    while (!WindowShouldClose())
    {
        updateUnicornEEGSource();
        updateTrialConductor();

        if (updateTinyBCIPipeline()) break;

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference))
        {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printf("%llu | Output received: %d (%.0f%% confidence)\t [", timestamp,
                inference.predictedLabel, inference.confidence * 100
            );

            for (int i=0; i<N_FREQS; i++)
                printf("%f", inference.confidences[i]);
            printf("]\n");

            // TODO code integration
            /* log to file */
            if (inferenceLog) {
                fprintf(inferenceLog, "%llu,%d,%d,%.6f",
                        timestamp,
                        getTarget(),      /* true label — current stimulus */
                        inference.predictedLabel,
                        inference.confidence);
                for (int i = 0; i < N_FREQS; i++)
                    fprintf(inferenceLog, ",%.6f", inference.confidences[i]);
                fprintf(inferenceLog, "\n");
                fflush(inferenceLog);
            }

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