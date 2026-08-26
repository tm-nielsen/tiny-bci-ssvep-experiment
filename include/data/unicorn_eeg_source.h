# pragma once
# include "storage.h"
# include "unicorn_producer.h"

# define SAMPLE_RATE 250.0
# define SAMPLE_INTERVAL (uint64_t)(1000000.0f / SAMPLE_RATE)


void initializeUnicornEEGSource(const char *port);
void resetUnicornEEGSource();
void updateUnicornEEGSource();
void closeUnicornEEGSource();

uint8_t getUnicornEEGSourceChannelCount();
uint32_t getUnicornEEGSourceSampleRate();