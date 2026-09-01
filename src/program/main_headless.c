# include "program/helpers.h"

# include "pipeline.h"
# include "inference_logger.h"
# include "microsecond_timer.h"

# include "lsl/inference_stream.h"
# include "lsl/trigger_stream.h"

# include "cli/eeg_source_selection.h"
# include "cli/helpers.h"

static uint16_t currentTargetLabel = 0;
static void onTriggerReceived(uint16_t code)
{
    uint64_t pushTimestamp = pushTrigger(code);
    if (code == TRIAL_END_CODE) return; // nothing to do at trial boundaries here
    currentTargetLabel = code - 1; // matches presenter's pushTrigger(target + 1)
    notifyInferenceLoggerOfNewTarget(currentTargetLabel, pushTimestamp);
}

// ---

static void cleanUp(void)
{
    cleanUpEEGSourceAndPipeline();
    closeLslTriggerSource();
    closeLslInferenceOutlet();
}

static void handleDisconnection(const char *message)
{
    printf("---\n%s\n\n", message);
    cleanUp();
    printf("\npress [Enter] to quit\n");

    awaitCLINewline();
    exit(EXIT_SUCCESS);
}

static void updateProgram(void)
{    
    if (!isSelectedEEGSourceConnected())
    {
        handleDisconnection("EEG Source Disconnected");
    }

    updatePipeline(&cleanUp);
}

int main(void)
{
    runEEGSourceSelection();

    initializeSelectedEEGSource();
    initializePipelineWithEEGSourceParameters();
    initializeLslTriggerSource(&onTriggerReceived);
    startUpdateThreadForSelectedEEGSource();

    printf("---\nSearching for Presenter App...\n");
    awaitConnection(
        &isLslTriggerSourceConnected,
        &updateProgram,
        &tryConnectLslTriggerSource
    ); 
    printf("Connected to presenter app\n");
    printf("---\n");

    awaitFilterStabilization(&cleanUp);

    openLslInferenceOutlet();

    while (true)
    {
        updateLslTriggerSource();
   
        if (!isLslTriggerSourceConnected())
        {
            handleDisconnection("Presenter App Disconnected");
        }

        updateProgram();

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference)) {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printInference(inference, timestamp);
            logInference(inference, timestamp);
            pushLslInference(&inference, timestamp, currentTargetLabel);
        }
    }

    cleanUp();
    return EXIT_SUCCESS;
}