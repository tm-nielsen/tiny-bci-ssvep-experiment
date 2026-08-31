# pragma once

# define SIGNAL_FREQUENCY 10.0f
# define SIGNAL_AMPLITUDE 1.0f
# define NOISE_AMPLITUDE 0.3f
# define NOISE_60HZ_AMPLITUDE 0.5f

void initializeSyntheticEEGSource(uint8_t channelCount, uint32_t sampleRate);
void updateSyntheticEEGSource(void);
void closeSyntheticEEGSource(void);

bool isSyntheticEEGSourceReady(void);
uint8_t getSyntheticEEGSourceChannelCount(void);
uint32_t getSyntheticEEGSourceSampleRate(void);