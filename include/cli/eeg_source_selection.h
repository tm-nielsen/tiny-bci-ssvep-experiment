# pragma once
# ifndef EEG_SOURCES
# define EEG_SOURCES

void runEEGSourceSelection(void);

void initializeSelectedEEGSource(void);
void updateSelectedEEGSource(void);
void cleanUpSelectedEEGSource(void);

typedef enum {
    LSL_SOURCE,
    NEUROPAWN_SOURCE,
    UNICORN_SOURCE,
    DSI7_FLEX_SOURCE,
    DSI7_SOURCE,
    SYNTHETIC_SOURCE
} EEGSourceType;

EEGSourceType promptEEGSourceSelection(void);
const char * promptSerialPortSelection();

bool isSelectedEEGSourceConnected(void);
uint8_t getChannelCountOfSelectedEEGSource(void);
uint32_t getSampleRateOfSelectedEEGSource(void);

bool shouldStreamSelectedEEGSource(void);

# endif