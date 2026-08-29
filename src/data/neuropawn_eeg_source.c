# include "data/neuropawn_eeg_source.h"
# include "serial/data_source.h"
# include "pipeline.h"

static SerialDataSource dataSource;

static float eegScale;
static float sampleBuffer[NEUROPAWN_EEG_CHANNEL_COUNT];

static uint8_t expectedSampleIndex = 0;
static bool sampleIndexExpectationSet = false;

typedef enum {
    EXG_STATUS_VALID,
    EXG_STATUS_MISALIGNED,
    EXG_STATUS_UNEXPECTED_SAMPLE_INDEX
} EXGStatus;

// ---



// ---

static EXGStatus validateEXGFrame(SerialFrame frame)
{
    if (
        frame.buffer[0] != NEUROPAWN_START_BYTES[0] || 
        frame.buffer[frame.size - 1] != NEUROPAWN_END_BYTES[0]
    ) {
        fprintf(stderr, "neuropawn: payload invalid, frame is misaligned\n");
        return EXG_STATUS_MISALIGNED;
    }

    uint8_t frameIndex = frame.buffer[1];
    if (sampleIndexExpectationSet && frameIndex != expectedSampleIndex)
    {
        fprintf(stderr,
            "neuropawn: payload index %u doesn't "
            "match expected index of %u\n",
            frameIndex, expectedSampleIndex
        );
        expectedSampleIndex = frameIndex + 1;
        return EXG_STATUS_UNEXPECTED_SAMPLE_INDEX;
    }
    expectedSampleIndex = frameIndex + 1;
    sampleIndexExpectationSet = true;
    return EXG_STATUS_VALID;
}

static void parseEXG(uint8_t *frameBuffer, float *sampleBuffer)
{
    for (size_t channelIndex = 0; channelIndex < NEUROPAWN_EEG_CHANNEL_COUNT; channelIndex++)
    {
        int16_t raw = (int16_t)(((uint16_t)frameBuffer[1 + 2 * channelIndex] << 8) |
                                 (uint16_t)frameBuffer[2 + 2 * channelIndex]);
        sampleBuffer[channelIndex] = (float)raw * eegScale;
    }
}

static void parseAndPushNeuropawnFrame(SerialFrame frame)
{
    if (validateEXGFrame(frame) == EXG_STATUS_MISALIGNED) return;
    parseEXG(frame.buffer, sampleBuffer);
    pushEEGSampleToTinyBCIPipeline(sampleBuffer, frame.buffer[1]);
}

void connectNeuropawnEEGSource(const char *port, NeuropawnConfiguration config)
{
    printf("neuropawn: attempting to connect on %s\n", port);

    SerialHandle handle = INVALID_HANDLE_VALUE;
    if (serialOpen(&handle, port, config.timeout)) exit(EXIT_SUCCESS);
    printf("neuropawn: connected on %s\n", port);
    eegScale = NEUROPAWN_EEG_SCALE(config.gain);

    printf("neuropawn: attempting to detect board type...\n");
    NeuroPawnBoardType boardType = detectNeuropawnBoardType(&handle);
    if (boardType == NEUROPAWN_BOARD_UNKNOWN)
    {
        fprintf(stderr, "neuropawn: detection failed - no valid packets.\n");
        printf("---\nTry again after unplugging the device and plugging it in again\n");
        serialClose(&handle);
        exit(EXIT_SUCCESS);
    }

    uint16_t frameSize = boardType == NEUROPAWN_BOARD_IMU
        ? NEUROPAWN_IMU_FRAME_SIZE
        : NEUROPAWN_EEG_FRAME_SIZE;
    const char * typeString = (boardType == NEUROPAWN_BOARD_IMU) ? "IMU" : "non-IMU";
    printf("neuropawn: %s board detected\n", typeString);

    dataSource = createSerialDataSourceFromOpenHandle(
        handle, frameSize,
        NEUROPAWN_START_BYTES,
        NEUROPAWN_END_BYTES
    );

    printf("neuropawn: configuring channels (gain %u)...\n", config.gain);
    configureNeuropawnChannels(&handle, frameSize, config);

    setSerialDataSourceCallback(&dataSource, &parseAndPushNeuropawnFrame);
}

void updateNeuropawnEEGSource()
{
    updateSerialDataSource(&dataSource);
}

void closeNeuropawnEEGSource() { closeSerialDataSource(&dataSource); }

bool isNeuropawnEEGSourceConnected() { return isSerialDataSourceConnected(&dataSource); }
uint8_t getNeuropawnEEGSourceChannelCount() { return NEUROPAWN_EEG_CHANNEL_COUNT; }
uint32_t getNeuropawnEEGSourceSampleRate() { return NEUROPAWN_SAMPLE_RATE; }