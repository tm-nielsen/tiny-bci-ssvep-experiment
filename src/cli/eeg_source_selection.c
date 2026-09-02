# include "cli/eeg_source_selection.h"
# include "cli/recording_options.h"
# include "cli/helpers.h"
# include "serial/port_enumeration.h"
# include "data/synthetic_eeg_source.h"
# include "data/neuropawn_eeg_source.h"
# include "data/unicorn_eeg_source.h"
# include "data/dsi_eeg_source.h"
# include "lsl/eeg_source.h"

static void (*initializationMethod)(void);
static void (*updateMethod)(void);
static void (*cleanupMethod)(void);

static bool (*connectionPredicate)(void);
static uint8_t (*channelCountGetMethod)(void);
static uint32_t (*sampleRateGetMethod)(void);

static char selectedPortName[MAXIMUM_PORT_NAME_LENGTH];
static bool selectedSourceShouldBeStreamed = false;

static void safeInvoke(void (*method)(void))
{
    if (method != NULL) method();
    else
    fprintf(stderr, "Error: can't invoke null method\n");
}

void initializeSelectedEEGSource(void) { safeInvoke(initializationMethod); }
void updateSelectedEEGSource(void) { safeInvoke(updateMethod); }
void cleanUpSelectedEEGSource(void) { safeInvoke(cleanupMethod); }

// ---

static const uint8_t testEEGChannelCount = 8;
static const uint32_t testEEGSampleRate = 250;

static void initializeTestSource(void)
{
    initializeSyntheticEEGSource(
        testEEGChannelCount,
        testEEGSampleRate
    );
}

static const uint32_t serialTimeout = 0;

static void initializeNeuropawnSource(void)
{
    connectNeuropawnEEGSource(
        selectedPortName,
        (NeuropawnConfiguration)
        { 
            .gain = 12, .timeout = serialTimeout,
            .activateChannel = TRUE_8_ARRAY,
            .activateRightLegDrive = FALSE_8_ARRAY
        }
    );
}

static void initializeUnicornSource(void)
{
    connectUnicornEEGSource(selectedPortName, serialTimeout);
}

static void initializeDsi7Source(void)
{
    connectDsiEEGSource(selectedPortName, DSI_7_MONTAGE);
}

// ---

static void selectEEGSource(unsigned int selection)
{
    if (selection == DSI7_SOURCE && !isDsiLibraryAvailable())
    {
        printf("DSI API Library not present at %s\n", DSI_LIBRARY_PATH);
        getchar();
        exit(EXIT_SUCCESS);
    }
    if (selection != LSL_SOURCE && selection != SYNTHETIC_SOURCE)
    {
        strcpy(selectedPortName, promptSerialPortSelection());
    }

    switch (selection)
    {
        case LSL_SOURCE:
            initializationMethod = connectLslEEGSource;
            updateMethod = updateLslEEGSource;
            cleanupMethod = closeLslEEGSource;
            connectionPredicate = isLslEEGSourceConnected;
            channelCountGetMethod = getLslEEGSourceChannelCount;
            sampleRateGetMethod = getLslEEGSourceSampleRate;
        break;

        case NEUROPAWN_SOURCE:
            initializationMethod = initializeNeuropawnSource;
            updateMethod = updateNeuropawnEEGSource;
            cleanupMethod = closeNeuropawnEEGSource;
            connectionPredicate = isNeuropawnEEGSourceConnected;
            channelCountGetMethod = getNeuropawnEEGSourceChannelCount;
            sampleRateGetMethod = getNeuropawnEEGSourceSampleRate;
        break;

        case UNICORN_SOURCE:
            initializationMethod = initializeUnicornSource;
            updateMethod = updateUnicornEEGSource;
            cleanupMethod = closeUnicornEEGSource;
            connectionPredicate = isUnicornEEGSourceConnected;
            channelCountGetMethod = getUnicornEEGSourceChannelCount;
            sampleRateGetMethod = getUnicornEEGSourceSampleRate;
        break;

        case DSI7_SOURCE:
            initializationMethod = initializeDsi7Source;
            updateMethod = updateDsiEEGSource;
            cleanupMethod = disconnectDsiEEGSource;
            connectionPredicate = isDsiEEGSourceConnected;
            channelCountGetMethod = getDsiEEGSourceChannelCount;
            sampleRateGetMethod = getDsiEEGSourceSampleRate;
        break;

        case SYNTHETIC_SOURCE:
            initializationMethod = initializeTestSource;
            updateMethod = updateSyntheticEEGSource;
            cleanupMethod = closeSyntheticEEGSource;
            connectionPredicate = isSyntheticEEGSourceReady;
            channelCountGetMethod = getSyntheticEEGSourceChannelCount;
            sampleRateGetMethod = getSyntheticEEGSourceSampleRate;
        break;
    }
}

void runEEGSourceSelection(void)
{
    EEGSourceType type = promptEEGSourceSelection();
    selectEEGSource(type);

    if (shouldStreamEEGAndTriggers())
    {
        switch (type)
        {
            case SYNTHETIC_SOURCE:
                printf(
                    "LSL outlet will not be opened"
                    " to stream synthetic data\n"
                );
            break;
            case LSL_SOURCE: break;

            default:
                selectedSourceShouldBeStreamed = true;
            break;
        }
    }
    printHorizontalRule();
}

// ---

bool isSelectedEEGSourceConnected(void)
{
    if (connectionPredicate == NULL) return 0;
    return connectionPredicate();
}

uint8_t getChannelCountOfSelectedEEGSource(void)
{
    if (channelCountGetMethod == NULL) return 0;
    return channelCountGetMethod();
}
uint32_t getSampleRateOfSelectedEEGSource(void)
{
    if (sampleRateGetMethod == NULL) return 0;
    return sampleRateGetMethod();
}

bool shouldStreamSelectedEEGSource(void) {
    return selectedSourceShouldBeStreamed;
}