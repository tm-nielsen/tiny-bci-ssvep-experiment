# include "cli/program_mode_selection.h"
# include "cli/helpers.h"

# include "program/constants.h"
# include "program/helpers.h"
# include "presentation.h"
# include "trial_conductor.h"
# include "microsecond_timer.h"

# include "pipeline.h"
# include "triggers.h"
# include "lsl/trigger_stream.h"
# include "lsl/inference_stream.h"
# include "inference_logger.h"

# include "cli/eeg_source_selection.h"

static ProgramMode programMode;
# define STANDALONE(code) if(programMode == Standalone) { code }
# define PRESENTATION_ONLY(code) if(programMode == PresentationOnly) { code }

ProgramMode promptProgramModeSelection()
{
    printf("Select program mode\n");
    printf("\t%u - Standalone\n", Standalone);
    printf(
        "\t%u - Presentation Only "
        "(connected to headless engine over LSL)\n"
        , PresentationOnly
    );

    uint32_t selection = getIntegerSelection(PresentationOnly);

    printf(
        "\nProceeding in %s mode...\n",
        selection == Standalone ? "Standalone" : "Presentation Only"
    );
    return (ProgramMode)selection;
}

// ---

static uint16_t currentTargetLabel = 0;
void onTrialStart(uint16_t target)
{
    currentTargetLabel = target;
    STANDALONE(pushTrigger(target + 1);)
    pushLslTrigger(target + 1);
    setPresentationTarget(target);
    resumeStimulus();
}

void onTrialEnd(uint16_t nextTarget)
{
    STANDALONE(pushTrigger(TRIAL_END_CODE);)
    pushLslTrigger(TRIAL_END_CODE);
    setPresentationTarget(nextTarget);
    pauseStimulus();
}

static bool allTrialsCompleted = false;
void onAllTrialsCompleted()
{
    allTrialsCompleted = true;
    clearPresentationTarget();
}

bool tryGetInference(TinyBCIInference *inference, uint64_t *timestamp)
{
    STANDALONE(
        *timestamp = getCurrentMicrosecondTimestamp();
        return tryGetTinyBCIInference(inference);
    )
    PRESENTATION_ONLY(
        return pollLslInferenceSource(inference, timestamp);
    )
    return false;
}

// ---

void displayHeadlessRuntimeConnectionWaitMessage()
{
    displayMessageOrExit("Waiting for BCI Engine...", &cleanUpProgram);
}

void connectToHeadlessRuntime()
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
    STANDALONE(
        drawMessageScreen("Initializing EEG Source...");
        initializeSelectedEEGSource();
    )
    openLslTriggerOutlet();

    STANDALONE(
        initializePipelineWithEEGSourceParameters();
        drawMessageScreen("Awaiting Filter Stabilization...");
        awaitFilterStabilization(&cleanUpProgram);
    )

    PRESENTATION_ONLY(connectToHeadlessRuntime();)
}

void awaitPromptedProgramStart()
{
    while (!IsKeyPressed(KEY_SPACE))
    {
        drawPreparationScreen("Press Spacebar to Start");
        STANDALONE(
            if (!isSelectedEEGSourceConnected()) return;
            updateEEGSourceAndPipeline(&cleanUpProgram);
        )
        closeIfPromptedTo(&cleanUpProgram);
    }
}

void updateProgram()
{
    if (allTrialsCompleted)
    {
        drawMessageScreen("Experiment Complete");
        return;
    }

    PRESENTATION_ONLY(
        if (!isLslInferenceSourceConnected())
        {
            drawMessageScreen("Connection Lost");
            return;
        }
    )

    STANDALONE(
        if (!isSelectedEEGSourceConnected())
        {
            drawMessageScreen("EEG Source Disconnected");
            return;
        }
    )

    updateTrialConductor();
    STANDALONE(updateEEGSourceAndPipeline(&cleanUpProgram);)

    TinyBCIInference inference;
    uint64_t timestamp;
    if (tryGetInference(&inference, &timestamp))
    {
        displayInference(inference, timestamp);
        STANDALONE(logInference(inference, timestamp, currentTargetLabel);)
    }

    drawStimulusScreen();
}

void cleanUpProgram()
{
    STANDALONE(cleanUpEEGSourceAndPipeline();)
    PRESENTATION_ONLY(closeLslInferenceSource();)
    closeLslTriggerOutlet();
    stopPresentation();
}