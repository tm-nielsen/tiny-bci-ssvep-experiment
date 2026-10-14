# include <signal.h>
# include <unistd.h>
# include <time.h>

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
    (void)signalNumber;
    if (stopRequested) _exit(EXIT_FAILURE); // second Ctrl+C: leave immediately
    stopRequested = 1;
}

static void installStopHandlers(void)
{
# if defined(_WIN32) || defined(_WIN64)
    signal(SIGINT, onStopSignal);
# else
    struct sigaction action = { 0 };
    action.sa_handler = onStopSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0; // no SA_RESTART: interrupts the poll() wait
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGHUP, &action, NULL);
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
    waitForSelectedEEGSourceData(IDLE_TIMEOUT_MS);
    updateSelectedEEGSource();

    if (!isSelectedEEGSourceConnected())
    {
        handleDisconnection("EEG Source Disconnected");
    }

    updatePipeline(&cleanUp);
}

static void awaitFilterStabilizationSingleThread(void)
{
    printf("Waiting for filter to settle...\n");
    MicrosecondTimer stabilizationTimer = createMicrosecondTimer(FILTER_STABILIZATION_DELAY);
    resetMicrosecondTimer(&stabilizationTimer);

    while (!checkMicrosecondTimer(&stabilizationTimer)) updateProgram();
    printf("Filter settled.\n");
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
    //startUpdateThreadForSelectedEEGSource();
    initializeSignalBufferMutex();

    printf("Searching for Presenter App...\n");
    awaitConnection(
        &isLslTriggerSourceConnected,
        &updateProgram,
        &tryConnectLslTriggerSource
    ); 
    printf("Connected to presenter app\n");
    printHorizontalRule();

    awaitFilterStabilizationSingleThread();

    while (!stopRequested)
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