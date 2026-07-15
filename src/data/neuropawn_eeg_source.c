# include "data/neuropawn_eeg_source.h"
# include "data/serial.h"
# include "microsecond_timer.h"

static SerialHandle handle = INVALID_HANDLE_VALUE;

static uint8_t payload[NEUROPAWN_IMU_PAYLOAD_LEN];
static uint8_t payloadLength;
static uint8_t payloadCursor = 0;

static float eegScale;
static float samples[CHANNEL_COUNT];
static uint32_t sampleIndex = 0;
static uint8_t expectedSampleIndex = 0;
static bool sampleIndexExpectationSet = false;

// ---

void sendCommand(const char *command)
{
    serialWrite(&handle, (uint8_t *)command, strlen(command));
    sleepMilliseconds(NEUROPAWN_CMD_PAUSE_MS);
    if (awaitSerialData(&handle))
    {
        printf("neuropawn: configuration command failed, retrying\n");
        sendCommand(command);
    }
}

// ---

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

void resetPayload()
{
    for (int i = 0; i < payloadLength; i++) payload[i] = 0;
    payloadCursor = 0;
}

typedef enum {
    READ_STATUS_READY,
    READ_STATUS_PENDING,
    READ_STATUS_INVALID
} ReadStatus;

ReadStatus readFrame()
{
    if (payloadCursor == 0)
    {
        uint16_t scanAttempts = 8192;
        while (payload[0] != NEUROPAWN_START_BYTE)
        {
            serialRead(&handle, payload, 1);
            if (scanAttempts-- <= 0)
            {
                fprintf(stderr, "neuropawn: failed to locate start byte\n");
                return READ_STATUS_INVALID;
            }
        }
        payloadCursor = 1;
    }

    int readCount = serialRead(&handle, payload + payloadCursor, payloadLength - payloadCursor);
    if (readCount > 0) payloadCursor += readCount;

    if (payloadCursor < payloadLength) return READ_STATUS_PENDING;
    else
    {
        if (payload[payloadLength - 1] != NEUROPAWN_END_BYTE)
        {
            fprintf(stderr, "neuropawn: dropped packet due to misaligned frame\n");
            serialRead(&handle, payload, 1); // offset frame
            resetPayload();
            return READ_STATUS_INVALID;
        }
        return READ_STATUS_READY;
    }
}

void parseEXG()
{
    if (payload[0] != NEUROPAWN_START_BYTE)
    {
        fprintf(stderr, "neuropawn: start byte mismatch in payload, frame is misaligned\n");
    }
    if (sampleIndexExpectationSet && payload[1] != expectedSampleIndex)
    {
        fprintf(stderr, "neuropawn: payload index %u doesn't match expected index of %u\n", payload[1], expectedSampleIndex);
    }
    expectedSampleIndex = payload[1] + 1;
    sampleIndexExpectationSet = true;

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

    sleepMilliseconds(NEUROPAWN_CMD_PAUSE_MS);
    awaitSerialData(&handle);

    /* 1. per-channel enable / disable */
    printf("neuropawn: configuring channels (gain %u)...\n", config.gain);
    for (size_t channelIndex = 0; channelIndex < CHANNEL_COUNT; channelIndex++)
    {
        int channelLabel = (int)channelIndex + 1;
        if (config.activateChannel[channelIndex])
        {
            snprintf(cmd, sizeof cmd, "chon_%d_%u", channelLabel, config.gain);
            printf("neuropawn: enabling channel %d\n", channelLabel);
        }
        else
        {
            snprintf(cmd, sizeof cmd, "choff_%d", channelLabel);
            printf("neuropawn: disabling channel %d\n", channelLabel);
        }
        sendCommand(cmd);
        
        /* 2. optional right-leg-drive */
        if (config.activateRightLegDrive[channelIndex]) {
            snprintf(cmd, sizeof cmd, "rldadd_%d", channelLabel);
            printf("neuropawn: enabling right leg drive for channel %d\n", channelLabel);
            sendCommand(cmd);
        }
    }

    /* 3. auto-detect board type from packet stride */
    printf("neuropawn: detecting board type from incoming packets...\n");
    NeuroPawnBoardType boardType = detectBoardType();
    if (boardType == NEUROPAWN_BOARD_UNKNOWN) {
        fprintf(stderr, "neuropawn: detection failed - no valid packets. "
                        "Enable at least one channel and retry.\n");
        serialClose(&handle);
        exit(EXIT_SUCCESS);
    }

    payloadLength = boardType == NEUROPAWN_BOARD_IMU
        ? NEUROPAWN_IMU_PAYLOAD_LEN
        : NEUROPAWN_EEG_PAYLOAD_LEN;

    serialFlush(&handle);

    const char * typeString = (boardType == NEUROPAWN_BOARD_IMU) ? "IMU" : "non-IMU";
    printf("neuropawn: connected on %s (%s board)\n", port, typeString);
}

void resetNeuropawnEEGSource()
{
    resetPayload();
    serialFlush(&handle);
}

void updateNeuropawnEEGSource()
{
    if (readFrame() != READ_STATUS_READY) return;

    parseEXG();
    uint64_t timestamp = getCurrentMicrosecondTimestamp();
    in_push_signal(&tbciInputs, samples, timestamp, sampleIndex++);

    resetPayload();
}

void disconnectNeuropawnEEGSource() { serialClose(&handle); }