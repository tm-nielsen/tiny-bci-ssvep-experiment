# pragma once
# ifndef EEG_SOURCES
# define EEG_SOURCES

void runEEGSourceSelection();

void initializeEEGSource();
void updateEEGSource();
void cleanUpEEGSource();

typedef enum {
    LSLSource,
    NeuropawnSource,
    UnicornSource,
    DSI7Source,
    SyntheticSource
} EEGSourceType;

EEGSourceType promptEEGSourceSelection();
const char* promptSerialPortSelection();

uint8_t getEEGChannelCount();
uint32_t getEEGSampleRate();

# endif