#include <unistd.h>

# include "pipeline.h"
# include "presentation.h"
# include "trial_conductor.h"
# include "microsecond_timer.h"

# include "data/trigger_source.h"
# include "data/lsl_trigger_outlet.h"
# include "data/synthetic_eeg_source.h"

void initializeEEGSource() {resetSyntheticEEGSource();} // { connectLslEEGSource(); }
void updateEEGSource() {updateSyntheticEEGSource();} //{ updateLslEEGSource(); }
void cleanUpEEGSource() {cleanUpSyntheticEEGSource();} // { disconnectLslEEGSource(); }


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
    const float frequencies[N_FREQS] = {7.5f, 8.57f, 10.0f, 12.0f}; //9.0f, 7.5f, 8.0f, 7.0f, 11.0f, 8.5f
    const float trialDuration = 12.0f;
    const float breakDuration = 3.0f;
    const float selectionDisplayConfidenceThreshold = 0.5f;

    initializeTrialConductor(N_FREQS, trialDuration, breakDuration, onTrialStart, onTrialEnd);
    initializePresentation(frequencies, N_FREQS);
    setPresentationTarget(0);

    initializeEEGSource();
    openLslTriggerOutlet("tBCI_Experiment_Triggers");

    if (initializeTinyBCIPipeline(frequencies)) return EXIT_FAILURE;

    while (!IsKeyPressed(KEY_SPACE))
    {
        drawEntryScreen();
        updateEEGSource();

        if (WindowShouldClose())
        {
            cleanUpEEGSource();
            closeLslTriggerOutlet();
            stopPresentation();
            return EXIT_SUCCESS;
        }
    }

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("---\nTiny BCI Pipeline Running.\n\n");


    printf("Settling filter — please wait 5 seconds...\n");
    MicrosecondTimer settleTimer = createMicrosecondTimer(5.0f);
    while (!checkMicrosecondTimer(&settleTimer)) {
        updateEEGSource();
        updateTinyBCIPipeline();  /* tick pipeline so filter runs */
    }
    printf("Filter settled — starting.\n");
    resetTrialConductorTimers();

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

    cleanUpEEGSource();
    closeLslTriggerOutlet();
    stopTinyBCIPipeline();
    stopPresentation();

    return EXIT_SUCCESS;
}