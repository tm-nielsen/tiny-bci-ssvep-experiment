# include "program/helpers.h"
# include "program/constants.h"

# include "inference_logger.h"
# include "microsecond_timer.h"

# include "cli/eeg_source_selection.h"
# include "cli/helpers.h"
# include "lsl/eeg_outlet.h"

void initializePipelineWithEEGSourceParameters(void)
{
    uint8_t channelCount = getChannelCountOfSelectedEEGSource();
    uint32_t sampleRate = getSampleRateOfSelectedEEGSource();

    TBCI_Status pipelineStatus = initializeTinyBCIPipeline(
        FREQUENCIES, channelCount, sampleRate
    );
    if (pipelineStatus != TBCI_OK) exit(EXIT_FAILURE);
    initializeInferenceLogger();

    if (shouldStreamSelectedEEGSource())
    {
        createAndConnectPipelineEEGOutlet();
    }

    pipelineStatus = startTinyBCIPipeline();
    if (pipelineStatus != TBCI_OK) exit(EXIT_FAILURE);
    printHorizontalRule();
    printf("Tiny BCI Pipeline Running.\n\n");
}

void updatePipeline(void (*cleanUpMethod)(void))
{
    lockEEGSourceMutex();
    TBCI_Status pipelineStatus = updateTinyBCIPipeline();
    unlockEEGSourceMutex();

    if (pipelineStatus != TBCI_OK)
    {
        if (cleanUpMethod != NULL) cleanUpMethod();
        exit(EXIT_SUCCESS);
    }
}

void awaitFilterStabilization(void (*cleanUpMethod)(void))
{
    printf("Waiting for filter to settle...\n");

    MicrosecondTimer stabilizationTimer = createMicrosecondTimer(FILTER_STABILIZATION_DELAY);
    resetMicrosecondTimer(&stabilizationTimer);

    while (!checkMicrosecondTimer(&stabilizationTimer)) {
        if (!isSelectedEEGSourceConnected()) return;
        updatePipeline(cleanUpMethod);
    }
    printf("Filter settled.\n");
}

void cleanUpEEGSourceAndPipeline(void)
{
    cleanUpEEGSourceUpdateThread();
    cleanUpSelectedEEGSource();
    cleanUpTinyBCIPipeline();
    closePipelineEEGOutlet();
    closeInferenceLogger();
}