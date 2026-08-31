# pragma once
# ifndef LSL_EEG_SOURCE
# define LSL_EEG_SOURCE

# define EEG_STREAM_PREDICATE "type='EEG' or type='eeg'"

void connectLslEEGSource(void);
void updateLslEEGSource(void);
void closeLslEEGSource(void);

bool isLslEEGSourceConnected(void);
uint8_t getLslEEGSourceChannelCount(void);
uint32_t getLslEEGSourceSampleRate(void);

# endif