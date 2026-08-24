# pragma once
# ifndef EEG_SOURCES
# define EEG_SOURCES

void initializeEEGSource();
void updateEEGSource();
void cleanUpEEGSource();

void promptEEGSourceSelection();

uint8_t getEEGChannelCount();
uint32_t getEEGSampleRate();

# endif