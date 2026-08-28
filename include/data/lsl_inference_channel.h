#ifndef LSL_INFERENCE_CHANNEL_H
#define LSL_INFERENCE_CHANNEL_H

#include <stdint.h>
#include "pipeline.h"

#define INFERENCE_CHANNEL_COUNT (4 + N_FREQS)
#define INFERENCE_STREAM_TYPE "Inference"
#define INFERENCE_STREAM_SOURCE_ID "tBCI_Inference"
#define INFERENCE_STREAM_NAME_DEFAULT "tBCI_Inference_Results"

// --- Runtime side (publisher) ---
int openLslInferenceOutlet(const char *streamName);
void pushLslInference(const TinyBCIInference *inference, uint64_t timestampUs, uint16_t targetLabel);
void closeLslInferenceOutlet(void);

// --- Presenter side (subscriber) ---
int openLslInferenceInlet(const char *streamName);
// non-blocking poll; returns 1 if a new sample was written into *out, 0 otherwise
int pollLslInference(TinyBCIInference *out, uint64_t *timestampUs, uint16_t *targetLabel);
void closeLslInferenceInlet(void);

bool isLslInferenceConnected(void);

#endif