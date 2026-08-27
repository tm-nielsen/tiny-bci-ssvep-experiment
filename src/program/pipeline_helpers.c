# include "program/helpers.h"
# include "program/constants.h"

# include "inference_logger.h"
# include "microsecond_timer.h"

# include "cli/eeg_source_selection.h"
# include "lsl/eeg_outlet.h"

void initializePipelineWithEEGSourceParameters()
{
    uint8_t channelCount = getChannelCountOfSelectedEEGSource();
    uint32_t sampleRate = getSampleRateOfSelectedEEGSource();

    TBCI_Status pipelineStatus = initializeTinyBCIPipeline(
        FREQUENCIES, channelCount, sampleRate
    );
    if (pipelineStatus != TBCI_OK) exit(EXIT_FAILURE);
    initializeInferenceLogger();

    if (
        isSelectedEEGSourceEligibleForLslStreaming()
        && promptEEGOutletUsageSelection()
    ) createAndConnectPipelineEEGOutlet();

    pipelineStatus = startTinyBCIPipeline();
    if (pipelineStatus != TBCI_OK) exit(EXIT_FAILURE);
    printf("---\nTiny BCI Pipeline Running.\n\n");
}

void updateEEGSourceAndPipeline(void (*cleanUpMethod)())
{
    updateSelectedEEGSource();
    TBCI_Status pipelineStatus = updateTinyBCIPipeline();
    if (pipelineStatus != TBCI_OK)
    {
        if (cleanUpMethod != NULL) cleanUpMethod();
        exit(EXIT_SUCCESS);
    }
}

void awaitFilterStabilization(void (*cleanUpMethod)())
{
    printf("Waiting for filter to settle...\n");

    MicrosecondTimer stabilizationTimer = createMicrosecondTimer(FILTER_STABILIZATION_DELAY);
    resetMicrosecondTimer(&stabilizationTimer);

    while (!checkMicrosecondTimer(&stabilizationTimer)) {
        if (!isSelectedEEGSourceConnected()) return;
        updateEEGSourceAndPipeline(cleanUpMethod);
    }
    printf("Filter settled.\n");
}

void cleanUpEEGSourceAndPipeline()
{
    cleanUpSelectedEEGSource();
    cleanUpTinyBCIPipeline();
    closePipelineEEGOutlet();
    closeInferenceLogger();
}