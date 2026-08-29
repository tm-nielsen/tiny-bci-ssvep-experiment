# pragma once
# ifndef NEUROPAWN_EEG_SOURCE
# define NEUROPAWN_EEG_SOURCE

# include "serial/serial.h"

# define NEUROPAWN_IMU_CHANNEL_COUNT 9
# define NEUROPAWN_EEG_CHANNEL_COUNT 8
# define NEUROPAWN_START_BYTE 0xA0
# define NEUROPAWN_END_BYTE 0xC0
# define NEUROPAWN_SAMPLE_RATE 125
# define NEUROPAWN_EEG_FRAME_SIZE 21
# define NEUROPAWN_IMU_FRAME_SIZE 57
# define NEUROPAWN_DEFAULT_GAIN 12
# define NEUROPAWN_CONFIGURATION_COMMAND_DELAY 200u

# define NEUROPAWN_START_BYTES MAKE_OXFF_TERMINATED_BYTE_ARRAY(NEUROPAWN_START_BYTE)
# define NEUROPAWN_END_BYTES MAKE_OXFF_TERMINATED_BYTE_ARRAY(NEUROPAWN_END_BYTE)
# define NEUROPAWN_EEG_SCALE(gain) (4.0f / (float)(pow(2, 15) - 1) / gain / 79.57f * 1000000.0f)

typedef enum {
    NEUROPAWN_BOARD_UNKNOWN,
    NEUROPAWN_BOARD_EEG,
    NEUROPAWN_BOARD_IMU
} NeuroPawnBoardType;

typedef struct {
    uint8_t gain;
    uint32_t timeout;
    bool activateChannel[NEUROPAWN_EEG_CHANNEL_COUNT];
    bool activateRightLegDrive[NEUROPAWN_EEG_CHANNEL_COUNT];
} NeuropawnConfiguration;

# define TRUE_8_ARRAY {true, true, true, true, true, true, true, true}
# define FALSE_8_ARRAY {false, false, false, false, false, false, false, false}
# define NEUROPAWN_DEFAULT_CONFIGURATION (NeuropawnConfiguration) { 12, 50, TRUE_8_ARRAY, FALSE_8_ARRAY }

void connectNeuropawnEEGSource(const char *, NeuropawnConfiguration);
void updateNeuropawnEEGSource();
void closeNeuropawnEEGSource();

bool isNeuropawnEEGSourceConnected();
uint8_t getNeuropawnEEGSourceChannelCount();
uint32_t getNeuropawnEEGSourceSampleRate();

NeuroPawnBoardType detectNeuropawnBoardType(SerialHandle *handle);

void configureNeuropawnChannels(
    SerialHandle *handle, uint16_t frameSize,
    NeuropawnConfiguration config
);

# endif