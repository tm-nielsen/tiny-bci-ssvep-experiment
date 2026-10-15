# pragma once
# include "pipeline.h"

typedef struct
{
    uint64_t timestamp_us;
    uint64_t delta_time_us;
    uint16_t true_label;       // 1-based
    uint16_t predicted_label;  // 1-based
    uint16_t round_number;
    float confidence;
    float probabilities[N_FREQS];
} InferenceRecord;

InferenceRecord makeInferenceRecord(TinyBCIInference inference, uint64_t timestamp);

void initializeInferenceLogger(void);
void notifyInferenceLoggerOfNewTarget(uint16_t target, uint64_t timestamp);
void logInference(TinyBCIInference inference, uint64_t timestamp);
void closeInferenceLogger(void);