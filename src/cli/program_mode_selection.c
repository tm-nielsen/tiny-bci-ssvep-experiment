# include "cli/program_mode_selection.h"
# include "cli/helpers.h"

# include "program/helpers.h"
# include "presentation.h"
# include "trial_conductor.h"
# include "microsecond_timer.h"

# include "pipeline.h"
# include "lsl/trigger_stream.h"
# include "lsl/inference_stream.h"
# include "inference_logger.h"

# include "cli/eeg_source_selection.h"

static ProgramMode programMode;
# define IF_STANDALONE(code) if(programMode == STANDALONE) { code }
# define IF_PRESENTATION_ONLY(code) if(programMode == PRESENTATION_ONLY) { code }

ProgramMode promptProgramModeSelection(void)
{
    printf("Select program mode\n");
    printf("\t%u - Standalone\n", STANDALONE);
    printf(
        "\t%u - Presentation Only "
        "(connected to headless engine over LSL)\n"
        , PRESENTATION_ONLY
    );

    uint32_t selection = getCLIIntegerSelection(PRESENTATION_ONLY);

    printf(
        "\nProceeding in %s mode...\n",
        selection == STANDALONE ? "Standalone" : "Presentation Only"
    );
    return (ProgramMode)selection;
}

// ---

static void onTrialStart(uint16_t target)
{
    IF_STANDALONE(
        uint64_t triggerTimestamp = pushTrigger(target + 1);
        notifyInferenceLoggerOfNewTarget(target, triggerTimestamp);
    )
    pushLslTrigger(target + 1);
    setPresentationTarget(target);
    resumeStimulus();
}

static void onTrialEnd(uint16_t nextTarget)
{
    IF_STANDALONE(pushTrigger(TRIAL_END_CODE);)
    pushLslTrigger(TRIAL_END_CODE);
    setPresentationTarget(nextTarget);
    pauseStimulus();
}

static bool allTrialsCompleted = false;
static void onAllTrialsCompleted(void)
{
    allTrialsCompleted = true;
    clearPresentationTarget();
}

static bool tryGetInference(TinyBCIInference *inference, uint64_t *timestamp)
{
    IF_STANDALONE(
        *timestamp = getCurrentMicrosecondTimestamp();
        return tryGetTinyBCIInference(inference);
    )
    IF_PRESENTATION_ONLY(
        return pollLslInferenceSource(inference, timestamp);
    )
    return false;
}

// ---

static void displayHeadlessRuntimeConnectionWaitMessage(void)
{
    displayMessageOrExit("Waiting for BCI Engine...", &cleanUpProgram);
}

static void connectToHeadlessRuntime(void)
{
    while (!doesLslTriggerOutletHaveConsumers())
    {
        displayMessageOrExit("Searching for BCI Engine...", &cleanUpProgram);
    }

    initializeLslInferenceSource();

    awaitConnection(
        &isLslInferenceSourceConnected,
        &displayHeadlessRuntimeConnectionWaitMessage,
        &tryConnectLslInferenceSource
    );
}

// ---

void initializeProgram(ProgramMode mode)
{
    programMode = mode;
    initializeTrialPresentation(
        &onTrialStart, &onTrialEnd,
        &onAllTrialsCompleted
    );
    IF_STANDALONE(
        drawMessageScreen("Initializing EEG Source...");
        initializeSelectedEEGSource();
    )
    openLslTriggerOutlet();

    IF_STANDALONE(
        initializePipelineWithEEGSourceParameters();
        startUpdateThreadForSelectedEEGSource();
        drawMessageScreen("Awaiting Filter Stabilization...");
        awaitFilterStabilization(&cleanUpProgram);
    )

    IF_PRESENTATION_ONLY(connectToHeadlessRuntime();)
}

void awaitPromptedProgramStart(void)
{
    while (!IsKeyPressed(KEY_SPACE))
    {
        drawPreparationScreen("Press Spacebar to Start");
        IF_STANDALONE(
            if (!isSelectedEEGSourceConnected()) return;
            updatePipeline(&cleanUpProgram);
        )
        IF_PRESENTATION_ONLY(
            if(!isLslInferenceSourceConsumable()) return;
        )
        closeIfPromptedTo(&cleanUpProgram);
    }
}

void updateProgram(void)
{
    if (allTrialsCompleted)
    {
        drawMessageScreen("Experiment Complete");
        return;
    }

    IF_PRESENTATION_ONLY(
        if (!isLslInferenceSourceConnected())
        {
            drawMessageScreen("Connection Lost");
            return;
        }
    )

    IF_STANDALONE(
        if (!isSelectedEEGSourceConnected())
        {
            drawMessageScreen("EEG Source Disconnected");
            return;
        }
    )

    updateTrialConductor();
    IF_STANDALONE(updatePipeline(&cleanUpProgram);)

    TinyBCIInference inference;
    uint64_t timestamp;
    if (tryGetInference(&inference, &timestamp))
    {
        displayInference(inference, timestamp);
        IF_STANDALONE(logInference(inference, timestamp);)
    }

    drawStimulusScreen();
}

void cleanUpProgram(void)
{
    IF_STANDALONE(cleanUpEEGSourceAndPipeline();)
    IF_PRESENTATION_ONLY(closeLslInferenceSource();)
    closeLslTriggerOutlet();
    stopPresentation();
}