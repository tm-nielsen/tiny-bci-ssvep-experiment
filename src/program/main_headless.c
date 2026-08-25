# include "program/helpers.h"

# include "pipeline.h"
# include "triggers.h"
# include "inference_logger.h"
# include "microsecond_timer.h"

# include "lsl/inference_stream.h"
# include "lsl/trigger_stream.h"

# include "cli/eeg_source_selection.h"

static uint16_t currentTargetLabel = 0;
static void onTriggerReceived(uint16_t code)
{
    pushTrigger(code);
    if (code == TRIAL_END_CODE) return; // nothing to do at trial boundaries here
    currentTargetLabel = code - 1; // matches presenter's pushTrigger(target + 1)
}

// ---

void cleanUp()
{
    cleanUpEEGSourceAndPipeline();
    closeLslTriggerSource();
    closeLslInferenceOutlet();
}

void updateProgram()
{
    updateEEGSourceAndPipeline(&cleanUp);
}

void handleTriggersDisconnected()
{
    printf("---\nPresenter App Disconnected\n\n");
    cleanUp();
    printf("\npress [Enter] to quit\n");

    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    getchar();
    exit(EXIT_SUCCESS);
}

int main(void)
{
    runEEGSourceSelection();

    initializeEEGSource();
    initializeLslTriggerSource(&onTriggerReceived);

    printf("---\nSearching for Presenter App...\n");
    awaitConnection(
        &isLslTriggerSourceConnected,
        &updateProgram,
        &tryConnectLslTriggerSource
    ); 
    printf("Connected to presenter app\n");
    printf("---\n");

    initializePipelineWithEEGSourceParameters();
    awaitFilterStabilization(&cleanUp);

    openLslInferenceOutlet();

    while (true)
    {
        updateLslTriggerSource();
   
        if (!isLslTriggerSourceConnected())
        {
            handleTriggersDisconnected();
        }

        updateProgram();

        TinyBCIInference inference;
        if (tryGetTinyBCIInference(&inference)) {
            uint64_t timestamp = getCurrentMicrosecondTimestamp();
            printInference(inference, timestamp);
            logInference(inference, timestamp, currentTargetLabel);
            pushLslInference(&inference, timestamp, currentTargetLabel);
        }
    }

    cleanUp();
    return EXIT_SUCCESS;
}