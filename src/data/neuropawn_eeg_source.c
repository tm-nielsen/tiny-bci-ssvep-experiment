# include "data/neuropawn_eeg_source.h"
# include "data/serial.h"
# include "microsecond_timer.h"

static SerialHandle handle = INVALID_HANDLE_VALUE;
static uint8_t payload[NEUROPAWN_IMU_PAYLOAD_LEN];
static uint8_t payloadLength;
static float eegScale;

static MicrosecondTimer timer = { .interval = SAMPLE_INTERVAL };
static float samples[CHANNEL_COUNT];
static uint32_t sampleIndex = 0;

// ---

void sendCommand(const char *command)
{
    serialWrite(&handle, (uint8_t *)command, strlen(command));
    sleepMilliseconds(NEUROPAWN_CMD_PAUSE_MS);
}

NeuroPawnBoardType scanFrameSize(const uint8_t *buffer, size_t bufferLength)
{
    for (size_t i = 0; i < bufferLength; i++) {
        if (buffer[i] != NEUROPAWN_START_BYTE)
            continue;

        /* non-IMU: stride 21 */
        if (i + 42 < bufferLength &&
            buffer[i + 20] == NEUROPAWN_END_BYTE   &&
            buffer[i + 21] == NEUROPAWN_START_BYTE &&
            buffer[i + 41] == NEUROPAWN_END_BYTE   &&
            buffer[i + 42] == NEUROPAWN_START_BYTE)
            return NEUROPAWN_BOARD_EEG;

        /* IMU: stride 57 */
        if (i + 114 < bufferLength &&
            buffer[i + 56]  == NEUROPAWN_END_BYTE   &&
            buffer[i + 57]  == NEUROPAWN_START_BYTE &&
            buffer[i + 113] == NEUROPAWN_END_BYTE   &&
            buffer[i + 114] == NEUROPAWN_START_BYTE)
            return NEUROPAWN_BOARD_IMU;
    }
    return NEUROPAWN_BOARD_UNKNOWN;
}

NeuroPawnBoardType detectBoardType()
{
    static uint8_t buffer[8192];
    size_t scanLength = 0;
    int attempts = 10;  /* ~10 s at 50 ms per read */

    serialFlush(&handle);

    while (attempts-- > 0) {
        if (scanLength >= sizeof(buffer)) {
            /* keep only the tail so detection stays bounded */
            memmove(buffer, buffer + scanLength - 1024, 1024);
            scanLength = 1024;
        }
        int readCount = serialRead(&handle, buffer + scanLength, sizeof(buffer) - scanLength);
        if (readCount > 0) {
            scanLength += (size_t)readCount;
            NeuroPawnBoardType type = scanFrameSize(buffer, scanLength);
            if (type != NEUROPAWN_BOARD_UNKNOWN)
                return type;
        }
    }
    return NEUROPAWN_BOARD_UNKNOWN;
}

// ---

int alignFrame()
{
    uint8_t scanByte = 0;
    uint16_t maximumScanAttempts = 8192;
    uint16_t attemptCounter = 0;

    while (attemptCounter++ < maximumScanAttempts)
    {
        int readCount = serialRead(&handle, &scanByte, 1);
        if (readCount == 1 && scanByte == NEUROPAWN_START_BYTE) break;
    }
    if (attemptCounter == maximumScanAttempts) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int readFrame()
{
    if (alignFrame()) return EXIT_FAILURE;

    size_t cursor = 0;
    uint8_t attempts = 200;
    while (cursor < payloadLength && attempts-- > 0)
    {
        int readCount = serialRead(&handle, payload + cursor, payloadLength - cursor);
        if (readCount > 0) cursor += (size_t)readCount;
    }
    if (cursor < payloadLength) return EXIT_FAILURE;
    if (payload[payloadLength - 1] != NEUROPAWN_END_BYTE) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

void parseEXG()
{
    for (size_t channelIndex = 0; channelIndex < CHANNEL_COUNT; channelIndex++)
    {
        int16_t raw = (int16_t)(((uint16_t)payload[1 + 2 * channelIndex] << 8) |
                                 (uint16_t)payload[2 + 2 * channelIndex]);
        samples[channelIndex] = (float)raw * eegScale;
    }
}

// ---

void connectNeuropawnEEGSource(const char *port, NeuropawnConfiguration config)
{
    if (SAMPLE_RATE != NEUROPAWN_SAMPLE_RATE)
    {
        fprintf(stderr, "Sample rate must be configured to match in 'eeg_source.h'\n");
        exit(EXIT_FAILURE);
    }

    if (serialOpen(&handle, port, config.timeout)) return;
    eegScale = (4.0f / 32767.0f / config.gain * 1000000.0f);
    char cmd[32];

    /* 1. per-channel enable / disable */
    printf("neuropawn: configuring channels (gain %u)...\n", config.gain);
    for (size_t channelIndex = 0; channelIndex < CHANNEL_COUNT; channelIndex++)
    {
        int channelLabel = (int)channelIndex + 1;
        if (config.activateChannel[channelIndex])
            snprintf(cmd, sizeof cmd, "chon_%d_%u", channelLabel, config.gain);
        else
            snprintf(cmd, sizeof cmd, "choff_%d", channelLabel);
        sendCommand(cmd);
        
        /* 2. optional right-leg-drive */
        if (config.activateRightLegDrive[channelIndex]) {
            snprintf(cmd, sizeof cmd, "rldadd_%d", channelLabel);
            sendCommand(cmd);
        }
    }

    /* 3. auto-detect board type from packet stride */
    printf("neuropawn: detecting board type from incoming packets...\n");
    NeuroPawnBoardType boardType = detectBoardType();
    if (boardType == NEUROPAWN_BOARD_UNKNOWN) {
        fprintf(stderr, "neuropawn: detection failed — no valid packets. "
                        "Enable at least one channel and retry.\n");
        serialClose(&handle);
        exit(EXIT_SUCCESS);
    }

    payloadLength = boardType == NEUROPAWN_BOARD_IMU
        ? NEUROPAWN_IMU_PAYLOAD_LEN
        : NEUROPAWN_EEG_PAYLOAD_LEN;

    serialFlush(&handle);
    readFrame();

    const char * typeString = (boardType == NEUROPAWN_BOARD_IMU) ? "IMU" : "non-IMU";
    printf("neuropawn: connected on %s (%s board)\n", port, typeString);
}

void updateNeuropawnEEGSource()
{
    if (checkMicrosecondTimer(&timer))
    {
        uint64_t timestamp = getCurrentMicrosecondTimestamp();

        if (readFrame())
        {
            fprintf(stderr, "Failed to read frame from neuropawn board\n");
            return;
        }
        parseEXG();

        in_push_signal(&tbciInputs, samples, timestamp, sampleIndex++);
    }
}

void disconnectNeuropawnEEGSource() { serialClose(&handle); }