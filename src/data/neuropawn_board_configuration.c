# include "data/neuropawn_eeg_source.h"
# include "serial/data_source.h"

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

NeuroPawnBoardType detectNeuropawnBoardType(SerialHandle *handle)
{
    uint32_t bufferLength = 4 * NEUROPAWN_IMU_FRAME_SIZE;
    uint8_t *buffer = malloc(bufferLength);

    NeuroPawnBoardType type = NEUROPAWN_BOARD_UNKNOWN;
    uint16_t scanAttempts = 2000;

    while (type == NEUROPAWN_BOARD_UNKNOWN && scanAttempts-- > 0)
    {
        size_t bytesRead = 0;
        while (bytesRead != bufferLength)
        {
            if (isSerialHandleInvalid(handle)) exit(EXIT_SUCCESS);
            bytesRead += serialRead(handle, buffer + bytesRead, bufferLength - bytesRead);   
        }

        type = scanFrameSize(buffer, bufferLength);
    }
    free(buffer);
    return type;
}

// ---

void sendCommand(SerialHandle *handle, const char *command)
{
    serialWrite(handle, (uint8_t *)command, strlen(command));
    serialFlush(handle);
}

void configureChannel(
    SerialDataSource *target, const char* cmd,
    uint8_t channelIndex, bool expectNonZeroSamples
)
{
    sendCommand(&(target->handle), cmd);
    sleepMilliseconds(NEUROPAWN_CONFIGURATION_COMMAND_DELAY);

    SerialFrame frame = awaitSerialDataSourceFrame(target);

    if (!expectNonZeroSamples) return;
    else if (
        frame.buffer[1 + 2 * channelIndex] ||
        frame.buffer[2 + 2 * channelIndex]
    ) return;
    else
    {
        fprintf(stderr, "neuropawn: failed to configure channel, retrying\n");
        configureChannel(target, cmd, channelIndex, expectNonZeroSamples);
    }
}

void configureNeuropawnChannels(
    SerialHandle *handle, uint16_t frameSize,
    NeuropawnConfiguration config
)
{
    char cmd[32];
    SerialDataSource dataSource = createSerialDataSourceFromOpenHandle(
        *handle, frameSize,
        NEUROPAWN_START_BYTES,
        NEUROPAWN_END_BYTES
    );

    for (uint8_t channelIndex = 0; channelIndex < NEUROPAWN_EEG_CHANNEL_COUNT; channelIndex++)
    {
        int channelLabel = (int)channelIndex + 1;
        bool channelEnabled = config.activateChannel[channelIndex];
        
        /* per-channel enable / disable */
        if (channelEnabled)
        {
            snprintf(cmd, sizeof cmd, "chon_%d_%u", channelLabel, config.gain);
            printf("neuropawn: enabling channel %d\n", channelLabel);
        }
        else
        {
            snprintf(cmd, sizeof cmd, "choff_%d", channelLabel);
            printf("neuropawn: disabling channel %d\n", channelLabel);
        }
        configureChannel(&dataSource, cmd, channelIndex, channelEnabled);
        
        /* optional right-leg-drive */
        if (config.activateRightLegDrive[channelIndex]) {
            snprintf(cmd, sizeof cmd, "rldadd_%d", channelLabel);
            printf("neuropawn: enabling right leg drive for channel %d\n", channelLabel);
            configureChannel(&dataSource, cmd, channelIndex, channelEnabled);
        }
    }
    closeSerialDataSourceButNotHandle(&dataSource);
}