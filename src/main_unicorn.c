# include <inttypes.h>
# include <unistd.h>
# include "pipeline.h"
# include "presentation.h"
# include "trial_conductor.h"
# include "microsecond_timer.h"
# include "data/lsl_trigger_outlet.h"
# include "data/trigger_source.h"
# include "data/unicorn_eeg_source.h"

# define PORT "/dev/cu.UN-20230805"

void onTrialStart(uint16_t target)
{
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


int main(int argc, char *argv[])
{
    const float frequencies[N_FREQS] = {7.5f, 8.57f, 10.0f, 12.0f}; //15, 10, 12, 8.5
    const float trialDuration = 20.0f;
    const float breakDuration = 3.0f;
    const float selectionDisplayConfidenceThreshold = 0.99f;

    initializeTrialConductor(N_FREQS, trialDuration, breakDuration, onTrialStart, onTrialEnd);
    initializePresentation(frequencies, N_FREQS);
    setPresentationTarget(0);

    openLslTriggerOutlet("tBCI_Experiment_Triggers");

    while (!IsKeyPressed(KEY_SPACE))
    {
        drawEntryScreen();

        if (WindowShouldClose())
        {
            closeLslTriggerOutlet();
            stopPresentation();
            return EXIT_SUCCESS;
        }
    }

    if (initializeTinyBCIPipeline(frequencies)) return EXIT_FAILURE;
    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("---\nTiny BCI Pipeline Running.\n\n");

    initializeUnicornEEGSource(PORT);
    resetUnicornEEGSource();

    /* settle filter with real data before starting trials */
    printf("Settling filter — please wait 5 seconds...\n");
    MicrosecondTimer settleTimer = createMicrosecondTimer(5.0f);
    while (!checkMicrosecondTimer(&settleTimer)) {
        updateUnicornEEGSource();
        updateTinyBCIPipeline();  /* tick pipeline so filter runs */
    }
    printf("Filter settled — starting.\n");
    resetTrialConductorTimers();


    while (!WindowShouldClose())
    {
        updateUnicornEEGSource();
        updateTrialConductor();

        if (updateTinyBCIPipeline()) break;

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference))
        {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printf("%" PRIu64 " | Output received: %d (%.0f%% confidence) [", timestamp,
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
                        inference.predictedLabel + 1,
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