# pragma once
# ifndef EEG_SOURCES
# define EEG_SOURCES

void runEEGSourceSelection();

void initializeSelectedEEGSource();
void updateSelectedEEGSource();
void cleanUpSelectedEEGSource();

typedef enum {
    LSL_SOURCE,
    NEUROPAWN_SOURCE,
    UNICORN_SOURCE,
    DSI7_SOURCE,
    SYNTHETIC_SOURCE
} EEGSourceType;

EEGSourceType promptEEGSourceSelection();
const char * promptSerialPortSelection();
bool promptEEGOutletUsageSelection();

bool isSelectedEEGSourceConnected();
uint8_t getChannelCountOfSelectedEEGSource();
uint32_t getSampleRateOfSelectedEEGSource();

bool shouldStreamSelectedEEGSource();

# endif