# include "data/unicorn_eeg_source.h"
# include "serial/data_source.h"
# include "pipeline.h"

static SerialDataSource dataSource;
static float sampleBuffer[UNICORN_EEG_CHANNEL_COUNT];
static uint32_t sampleIndex = 0;

// ---

static bool isFrameValid(SerialFrame frame)
{
    return frame.buffer[0] == UNICORN_START_BYTE0
    && frame.buffer[1] == UNICORN_START_BYTE1
    && frame.buffer[frame.size - 2] == UNICORN_STOP_BYTE0
    && frame.buffer[frame.size - 1] == UNICORN_STOP_BYTE1;
}

static void parseUnicornEEG(uint8_t *frameBuffer, float *sampleBuffer)
{
    for (uint8_t channelIndex = 0; channelIndex < UNICORN_EEG_CHANNEL_COUNT; channelIndex++)
    {
        /* 3 bytes big-endian signed 24-bit */
        int32_t raw = ((int32_t)frameBuffer[3 + 3 * channelIndex] << 16)
                    | ((int32_t)frameBuffer[4 + 3 * channelIndex] << 8)
                    | ((int32_t)frameBuffer[5 + 3 * channelIndex]);

        /* two's complement for 24-bit */
        if (raw & 0x00800000)
        {
            raw |= 0xFF000000;
            raw -= 0x01000000;
        }
        sampleBuffer[channelIndex] = (float)raw * UNICORN_EEG_SCALE;
    }
}

static void parseAndPushUnicornFrame(SerialFrame frame)
{
    if (!isFrameValid(frame))
    {
        fprintf(stderr, "Bad unicorn frame\n");
        return;
    }
    parseUnicornEEG(frame.buffer, sampleBuffer);
    pushEEGSampleToTinyBCIPipeline(sampleBuffer, sampleIndex++);
}

// ---

void connectUnicornEEGSource(const char *port, uint32_t timeout)
{
    dataSource = createSerialDataSource(
        UNICORN_PACKET_SIZE,
        UNICORN_START_BYTES,
        UNICORN_END_BYTES
    );
    if (openSerialDataSource(&dataSource, port, timeout)) exit(EXIT_FAILURE);
    SerialHandle handlePointer = &(dataSource.handle);

    serialWrite(handlePointer,
        UNICORN_START_ACQUISITION_COMMAND,
        UNICORN_COMMAND_LENGTH
    );
    serialFlush(handlePointer);

    uint8_t response[3];
    int attempts = 100;
    while (attempts-- > 0)
    {
        if (serialRead(handlePointer, response, 3) == 3) break;
    }

    if (
        response[0] == 0x00 &&
        response[1] == 0x00 &&
        response[2] == 0x00
    ) {
        setSerialDataSourceCallback(&dataSource, parseAndPushUnicornFrame);
    }
    else
    {
        fprintf(stderr, "Unicorn: Unexpected start response\n");
        closeSerialDataSource(&dataSource);
        exit(EXIT_FAILURE);
    }
}

void updateUnicornEEGSource()
{
    updateSerialDataSource(&dataSource);
}

void closeUnicornEEGSource()
{
    serialWrite(
        &(dataSource.handle),
        UNICORN_STOP_ACQUISITION_COMMAND,
        UNICORN_COMMAND_LENGTH
    );
    closeSerialDataSource(&dataSource);
}

bool isUnicornEEGSourceConnected() { return isSerialDataSourceConnected(&dataSource); }
uint8_t getUnicornEEGSourceChannelCount() { return UNICORN_EEG_CHANNEL_COUNT; }
uint32_t getUnicornEEGSourceSampleRate() { return UNICORN_SAMPLE_RATE; }