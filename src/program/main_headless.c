# include <signal.h>
# include "program/helpers.h"

# include "pipeline.h"
# include "inference_logger.h"
# include "microsecond_timer.h"

# include "lsl/inference_stream.h"
# include "lsl/trigger_stream.h"

# include "cli/eeg_source_selection.h"
# include "cli/recording_options.h"
# include "cli/helpers.h"
#include "program/constants.h"

# define IDLE_TIMEOUT_MS 8

static uint16_t currentTargetLabel = 0;

static void onTriggerReceived(uint16_t code, uint64_t timestamp)
{
    uint64_t pushTimestamp = pushTriggerWithTimestamp(code, timestamp);
    if (code == TRIAL_END_CODE) return; // nothing to do at trial boundaries here
    currentTargetLabel = code - 1; // matches presenter's pushTrigger(target + 1)
    notifyInferenceLoggerOfNewTarget(currentTargetLabel, pushTimestamp);
}

// ---

static volatile sig_atomic_t stopRequested = 0;

static void onStopSignal(int signalNumber)
{
    if (stopRequested) exit(EXIT_SUCCESS); // second stop signal: leave immediately
    stopRequested = 1;
    printf("\n-!-\n");
    signal(signalNumber, onStopSignal);
}

static void installStopHandlers(void)
{
    signal(SIGINT, onStopSignal);
    signal(SIGTERM, onStopSignal);
# if defined(SIGHUP)
    signal(SIGHUP, onStopSignal);
# endif
}

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

    updateEEGSourceAndPipeline(&cleanUp);
}

int main(void)
{
    installStopHandlers();
    openLslInferenceOutlet();

    runRecordingOptionSelection();
    runEEGSourceSelection();

    initializeSelectedEEGSource();

    printHorizontalRule();
    initializePipelineWithEEGSourceParameters();
    initializeLslTriggerSource(&onTriggerReceived);

    printf("Searching for Presenter App...\n");
    awaitConnection(
        &isLslTriggerSourceConnected,
        &updateProgram,
        &tryConnectLslTriggerSource
    ); 
    printf("Connected to presenter app\n");
    printHorizontalRule();

    awaitFilterStabilization(&cleanUp);

    while (!stopRequested)
    {
        updateLslTriggerSource();
   
        if (!isLslTriggerSourceConnected())
        {
            handleDisconnection("Presenter App Disconnected");
        }

        updateEEGSourceAndPipeline(&cleanUp);

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