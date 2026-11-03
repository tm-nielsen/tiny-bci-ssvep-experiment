# include "program/helpers.h"
# include "program/constants.h"

# include "inference_logger.h"
# include "microsecond_timer.h"

# include "cli/eeg_source_selection.h"
# include "cli/helpers.h"
# include "lsl/eeg_outlet.h"
# include "tick_logger.h"

#ifdef TBCI_MEASURE
# include "tick_logger.h"
# include "measurements/gpio_marker.h"
#endif

void initializePipelineWithEEGSourceParameters(void)
{
    uint8_t channelCount = getChannelCountOfSelectedEEGSource();
    uint32_t sampleRate = getSampleRateOfSelectedEEGSource();
    configureEEGTimestampAnchorSamplePeriod(sampleRate);

    TBCI_Status pipelineStatus = initializeTinyBCIPipeline(
        (float[])FREQUENCIES, channelCount, sampleRate
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

void updateEEGSourceAndPipeline(void (*cleanUpMethod)(void))
{
#ifdef TBCI_MEASURE
    setProfileMarker(true);
    uint64_t start = tickLoggerNowNs();
#endif

    updateSelectedEEGSource();
    TBCI_Status pipelineStatus = updateTinyBCIPipeline();

#ifdef TBCI_MEASURE
    uint64_t end = tickLoggerNowNs();
    setProfileMarker(false);
    bool decision = hasTinyBCIInference();
    setDecisionMarker(decision);
    logTick(start, end, decision);
#endif

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
        updateEEGSourceAndPipeline(cleanUpMethod);
    }
    printf("Filter settled.\n");
}

void cleanUpEEGSourceAndPipeline(void)
{
    cleanUpSelectedEEGSource();
    cleanUpTinyBCIPipeline();
    closePipelineEEGOutlet();
    closeInferenceLogger();
}