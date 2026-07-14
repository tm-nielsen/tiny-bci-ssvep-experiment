# pragma once
# include "storage.h"

float signalStorage[SIG_CAPACITY * CHANNEL_COUNT];
uint64_t signalTimestamps[SIG_CAPACITY];
uint32_t signalIndices[SIG_CAPACITY];

float processedSignalStorage[SIG_CAPACITY * CHANNEL_COUNT];
uint64_t processedSignalTimestamps[SIG_CAPACITY];
uint32_t processedSignalIndices[SIG_CAPACITY];

TBCI_Trigger triggerStorage[TRIG_CAPACITY];

TBCI_Epoch epochStorage[EPOCH_CAPACITY];
float epochPool[EPOCH_POOL_CAPACITY];
TBCI_Epoch featuresStorage[EPOCH_CAPACITY];
float featuresPool[EPOCH_POOL_CAPACITY];
TBCI_Epoch outputStorage[EPOCH_CAPACITY];
float outputPool[EPOCH_POOL_CAPACITY];

float ref_signals[REF_CAP];