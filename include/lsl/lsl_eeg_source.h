# pragma once
# ifndef LSL_EEG_SOURCE
# define LSL_EEG_SOURCE

# define LSL_EEG_PREDICATE "type='EEG' or type='eeg'"

void connectLslEEGSource();
void updateLslEEGSource();
void closeLslEEGSource();

uint8_t getLslEEGSourceChannelCount();
uint32_t getLslEEGSourceSampleRate();

# endif