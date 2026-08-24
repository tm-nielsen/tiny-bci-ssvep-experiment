# include "data/eeg_sources.h"
# include "data/serial_port_enumeration.h"
# include "data/synthetic_eeg_source.h"
# include "data/neuropawn_eeg_source.h"
# include "data/unicorn_eeg_source.h"
# include "lsl/eeg_source.h"

static void (*initializationMethod)();
static void (*updateMethod)();
static void (*cleanupMethod)();

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

const char* promptSerialPortSelection()
{
    while (true)
    {
        uint32_t deviceCount = enumerateSerialPorts();
        printf("%u Available Ports:\n", deviceCount);
        printf("\t0 : rescan\n");

        for (uint32_t i = 0; i < deviceCount; i++)
        {
            printf("\t%u : %s\n", i + 1, getSerialPortName(i));
        }
        printf("Selection one of options [0-%u]\t", deviceCount + 1);

        uint32_t selection;
        scanf("%u", &selection);

        if (selection == 0) continue;
        if (selection > deviceCount)
        {
            printf("Invalid selection\n Try Again\n");
            continue;
        }
        return getSerialPortName(selection - 1);
    }
}

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

typedef enum {
    LSLSource,
    NeuropawnSource,
    UnicornSource,
    DSI7Source,
    SyntheticSource
} EEGSourceType;

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
            channelCountGetMethod = getLslEEGSourceChannelCount;
            sampleRateGetMethod = getLslEEGSourceSampleRate;
        break;

        case NeuropawnSource:
            initializationMethod = initializeNeuropawnSource;
            updateMethod = updateNeuropawnEEGSource;
            cleanupMethod = closeNeuropawnEEGSource;
            channelCountGetMethod = getNeuropawnEEGSourceChannelCount;
            sampleRateGetMethod = getNeuropawnEEGSourceSampleRate;
        break;

        case UnicornSource:
            initializationMethod = initializeUnicornSource;
            updateMethod = updateUnicornEEGSource;
            cleanupMethod = closeUnicornEEGSource;
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
            channelCountGetMethod = getSyntheticEEGSourceChannelCount;
            sampleRateGetMethod = getSyntheticEEGSourceSampleRate;
        break;
    }
}

void promptEEGSourceSelection()
{
    printf("Select EEG Source\n");
    printf("\t%u - LSL Stream\n", LSLSource);
    printf("\t%u - Neuropawn over USB\n", NeuropawnSource);
    printf("\t%u - Unicorn over USB\n", UnicornSource);
    printf("\t%u - DSI-7 over USB\n", DSI7Source);
    printf("\t%u - Synthetic test data\n", SyntheticSource);

    unsigned int selection = 0;
    scanf("%u", &selection);

    while (selection > SyntheticSource)
    {
        printf("invalid selection\n");
        scanf("%u", &selection);
    }

    selectEEGSource(selection);
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