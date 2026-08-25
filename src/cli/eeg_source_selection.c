# include "cli/eeg_source_selection.h"
# include "data/serial_port_enumeration.h"
# include "data/synthetic_eeg_source.h"
# include "data/neuropawn_eeg_source.h"
# include "data/unicorn_eeg_source.h"
# include "lsl/eeg_source.h"

static void (*initializationMethod)();
static void (*updateMethod)();
static void (*cleanupMethod)();

static bool (*connectionPredicate)();
static uint8_t (*channelCountGetMethod)();
static uint32_t (*sampleRateGetMethod)();

static char selectedPortName[MAXIMUM_PORT_NAME_LENGTH];

void safeInvoke(void (*method)())
{
    if (method != NULL) method();
    else
    fprintf(stderr, "Error: can't invoke null method\n");
}

void initializeEEGSource() { safeInvoke(initializationMethod); }
void updateEEGSource() { safeInvoke(updateMethod); }
void cleanUpEEGSource() { safeInvoke(cleanupMethod); }

// ---

static const uint8_t testEEGChannelCount = 8;
static const uint32_t testEEGSampleRate = 250;

static void initializeTestSource()
{
    initializeSyntheticEEGSource(
        testEEGChannelCount,
        testEEGSampleRate
    );
}

static const uint32_t serialTimeout = 50;

static void initializeNeuropawnSource()
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

static void initializeUnicornSource() \
{
    connectUnicornEEGSource(selectedPortName, serialTimeout);
}

// ---

void selectEEGSource(unsigned int selection)
{
    if (selection != LSLSource && selection != SyntheticSource)
    {
        strcpy(selectedPortName, promptSerialPortSelection());
    }

    switch (selection)
    {
        case LSLSource:
            initializationMethod = connectLslEEGSource;
            updateMethod = updateLslEEGSource;
            cleanupMethod = closeLslEEGSource;
            connectionPredicate = isLslEEGSourceConnected;
            channelCountGetMethod = getLslEEGSourceChannelCount;
            sampleRateGetMethod = getLslEEGSourceSampleRate;
        break;

        case NeuropawnSource:
            initializationMethod = initializeNeuropawnSource;
            updateMethod = updateNeuropawnEEGSource;
            cleanupMethod = closeNeuropawnEEGSource;
            connectionPredicate = isNeuropawnEEGSourceConnected;
            channelCountGetMethod = getNeuropawnEEGSourceChannelCount;
            sampleRateGetMethod = getNeuropawnEEGSourceSampleRate;
        break;

        case UnicornSource:
            initializationMethod = initializeUnicornSource;
            updateMethod = updateUnicornEEGSource;
            cleanupMethod = closeUnicornEEGSource;
            connectionPredicate = isUnicornEEGSourceConnected;
            channelCountGetMethod = getUnicornEEGSourceChannelCount;
            sampleRateGetMethod = getUnicornEEGSourceSampleRate;
        break;

        case DSI7Source:
            printf("DSI-7 Support is not yet implemented\n");
            getchar();
            exit(EXIT_SUCCESS);
        break;

        case SyntheticSource:
            initializationMethod = initializeTestSource;
            updateMethod = updateSyntheticEEGSource;
            cleanupMethod = closeSyntheticEEGSource;
            connectionPredicate = isSyntheticEEGSourceReady;
            channelCountGetMethod = getSyntheticEEGSourceChannelCount;
            sampleRateGetMethod = getSyntheticEEGSourceSampleRate;
        break;
    }
}

void runEEGSourceSelection() { selectEEGSource(promptEEGSourceSelection()); }

bool isEEGSourceConnected()
{
    if (connectionPredicate == NULL) return 0;
    return connectionPredicate();
}

uint8_t getEEGChannelCount()
{
    if (channelCountGetMethod == NULL) return 0;
    return channelCountGetMethod();
}
uint32_t getEEGSampleRate()
{
    if (sampleRateGetMethod == NULL) return 0;
    return sampleRateGetMethod();
}