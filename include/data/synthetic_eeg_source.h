# pragma once
# include "storage.h"
# include "eeg_source.h"

# define SAMPLE_INTERVAL (uint64_t)(1000000.0f / SAMPLE_RATE)

# define SIGNAL_FREQUENCY 10.0f
# define SIGNAL_AMPLITUDE 1.0f
# define NOISE_AMPLITUDE 0.3f

void resetSyntheticEEGSource();
void updateSyntheticEEGSource();