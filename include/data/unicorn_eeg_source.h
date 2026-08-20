# pragma once
# include "storage.h"
# include "../thirdparty/tiny_bci/producer/unicorn_producer.h"

# define SAMPLE_RATE 250.0
# define SAMPLE_INTERVAL (uint64_t)(1000000.0f / SAMPLE_RATE)


void initializeUnicornEEGSource(const char *port);
uint8_t getUnicornEEGSourceChannelCount();
uint32_t getUnicornEEGSourceSampleRate();
void resetUnicornEEGSource();
void updateUnicornEEGSource();
void closeUnicornEEGSource();