# include "program/constants.h"
# include "program/helpers.h"

# include "pipeline.h"
# include "pipeline/storage.h"
# include "inference_logger.h"
# include "microsecond_timer.h"

# include "lsl/eeg_outlet.h"
# include "lsl/inference_stream.h"
# include "lsl/helpers.h"

# include "cli/eeg_source_selection.h"
# include "cli/recording_options.h"
# include "cli/helpers.h"
# include "serial/serial.h"

# include <time.h>

# if defined(_WIN32) || defined(_WIN64)
#   include <conio.h>
# else
#   include <sys/select.h>
#   include <termios.h>
#   include <unistd.h>
# endif

static const float STIMULUS_FREQUENCIES[N_FREQS] = {
    5.0f, 6.0f, 7.0f, 7.5f
};
static const uint32_t TRIAL_DURATION_SECONDS = 30;

# define MARKER_STREAM_NAME "Tiny_BCI_Experiment_Markers"
# define MARKER_STREAM_TYPE "Markers"
# define MARKER_STREAM_SOURCE_ID "tiny_bci_ssvep_experiment_markers"

static uint16_t currentTargetLabel = 0;
static uint32_t currentTrial = 0;
static lsl_outlet markerOutlet = NULL;
static bool experimentStarted = false;
static bool experimentEnded = false;
static bool trialInProgress = false;
static bool quitRequested = false;

# if !defined(_WIN32) && !defined(_WIN64)
static struct termios originalTerminalSettings;
static bool terminalSettingsChanged = false;
# endif

static void restoreTerminalInput(void)
{
# if !defined(_WIN32) && !defined(_WIN64)
    if (terminalSettingsChanged)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalTerminalSettings);
        terminalSettingsChanged = false;
    }
# endif
}

static void configureTerminalInput(void)
{
# if !defined(_WIN32) && !defined(_WIN64)
    if (tcgetattr(STDIN_FILENO, &originalTerminalSettings) != 0) return;

    struct termios terminalSettings = originalTerminalSettings;
    terminalSettings.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    terminalSettings.c_cc[VMIN] = 0;
    terminalSettings.c_cc[VTIME] = 0;
    terminalSettingsChanged = tcsetattr(
        STDIN_FILENO, TCSANOW, &terminalSettings
    ) == 0;
# endif
}

static int tryReadKey(void)
{
# if defined(_WIN32) || defined(_WIN64)
    return _kbhit() ? _getch() : -1;
# else
    fd_set inputSet;
    FD_ZERO(&inputSet);
    FD_SET(STDIN_FILENO, &inputSet);
    struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
    if (select(STDIN_FILENO + 1, &inputSet, NULL, NULL, &timeout) <= 0) return -1;

    unsigned char key;
    return read(STDIN_FILENO, &key, 1) == 1 ? key : -1;
# endif
}

static void pushMarker(
    const char *event, uint32_t trial, int target,
    float frequency, uint16_t triggerCode
)
{
    char marker[256];
    snprintf(
        marker, sizeof(marker),
        "participant=%s,session=%s,trial=%u,event=%s,target=%d,frequency_hz=%.2f,trigger_code=%u",
        tbciConfiguration.log_subject, tbciConfiguration.log_session,
        trial, event, target, frequency, triggerCode
    );
    pushLslStringSample(markerOutlet, marker);
    printf("%s\n", marker);
}

static void finishTrial(void)
{
    if (!trialInProgress) return;

    pushTrialEndCode();
    pushMarker(
        "trial_end", currentTrial, currentTargetLabel,
        STIMULUS_FREQUENCIES[currentTargetLabel], TRIAL_END_CODE
    );
    trialInProgress = false;
}

static void cleanUp(void)
{
    restoreTerminalInput();
    finishTrial();

    if (experimentStarted && !experimentEnded)
    {
        pushMarker("experiment_end", currentTrial, -1, 0.0f, 0);
        experimentEnded = true;
    }

    closeLslOutlet(&markerOutlet);
    cleanUpEEGSourceAndPipeline();
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

    TinyBCIInference inference;
    if (tryGetTinyBCIInference(&inference))
    {
        uint64_t timestamp = getCurrentMicrosecondTimestamp();
        printInference(inference, timestamp);
        logInference(inference, timestamp);
        pushLslInference(&inference, timestamp, currentTargetLabel);
    }
}

static void initializePipeline(void)
{
    uint8_t channelCount = getChannelCountOfSelectedEEGSource();
    uint32_t sampleRate = getSampleRateOfSelectedEEGSource();
    TBCI_Status status = initializeTinyBCIPipeline(
        STIMULUS_FREQUENCIES, channelCount, sampleRate
    );
    if (status != TBCI_OK) exit(EXIT_FAILURE);

    initializeInferenceLogger();
    if (shouldStreamSelectedEEGSource()) createAndConnectPipelineEEGOutlet();

    status = startTinyBCIPipeline();
    if (status != TBCI_OK) exit(EXIT_FAILURE);
    printHorizontalRule();
    printf("Tiny BCI Pipeline Running.\n\n");
}

static void shuffleTargets(uint16_t targets[N_FREQS])
{
    for (uint16_t target = 0; target < N_FREQS; target++)
    {
        targets[target] = target;
    }

    for (uint16_t remaining = N_FREQS; remaining > 1; remaining--)
    {
        uint16_t selected = (uint16_t)(rand() % remaining);
        uint16_t last = targets[remaining - 1];
        targets[remaining - 1] = targets[selected];
        targets[selected] = last;
    }
}

static bool waitForStartKey(void)
{
    while (true)
    {
        int key = tryReadKey();
        if (key == 's' || key == 'S') return true;
        if (key == 'q' || key == 'Q')
        {
            quitRequested = true;
            return false;
        }

        updateProgram();
        sleepMilliseconds(10);
    }
}

static void runTrial(uint16_t target, uint32_t trial)
{
    currentTargetLabel = target;
    currentTrial = trial;

    printf(
        "\nTrial %u/%u: look at the %.2f Hz stimulus. Press S to start, Q to quit.\n",
        trial, N_FREQS, STIMULUS_FREQUENCIES[target]
    );
    pushMarker("stimulus_selected", trial, target, STIMULUS_FREQUENCIES[target], 0);
    if (!waitForStartKey()) return;

    uint16_t triggerCode = target + 1;
    uint64_t triggerTimestamp = pushTrigger(triggerCode);
    notifyInferenceLoggerOfNewTarget(target, triggerTimestamp);
    trialInProgress = true;
    pushMarker(
        "trial_start", trial, target,
        STIMULUS_FREQUENCIES[target], triggerCode
    );

    uint64_t endTimestamp = getCurrentMicrosecondTimestamp()
        + (uint64_t)TRIAL_DURATION_SECONDS * 1000000;
    uint32_t lastCountdownValue = 0;

    while (true)
    {
        updateProgram();

        uint64_t now = getCurrentMicrosecondTimestamp();
        if (now >= endTimestamp) break;

        uint64_t remainingMicroseconds = endTimestamp - now;
        uint32_t remainingSeconds = (uint32_t)(
            (remainingMicroseconds + 999999) / 1000000
        );
        if (remainingSeconds <= 5 && remainingSeconds != lastCountdownValue)
        {
            printf("Trial ends in %u seconds...\n", remainingSeconds);
            lastCountdownValue = remainingSeconds;
        }
        sleepMilliseconds(10);
    }

    finishTrial();
}

int main(void)
{
    runRecordingOptionSelection();
    runEEGSourceSelection();

    initializeSelectedEEGSource();
    printHorizontalRule();
    initializePipeline();
    startUpdateThreadForSelectedEEGSource();
    awaitFilterStabilization(&cleanUp);

    openLslInferenceOutlet();
    markerOutlet = openIrregularRateLslOutlet(
        MARKER_STREAM_NAME, MARKER_STREAM_TYPE,
        1, cft_string, MARKER_STREAM_SOURCE_ID
    );

    srand((unsigned int)time(NULL));
    uint16_t targets[N_FREQS];
    shuffleTargets(targets);

    experimentStarted = true;
    pushMarker("experiment_start", 0, -1, 0.0f, 0);
    configureTerminalInput();

    for (uint32_t trial = 1; trial <= N_FREQS && !quitRequested; trial++)
    {
        runTrial(targets[trial - 1], trial);
    }

    cleanUp();
    return EXIT_SUCCESS;
}