# pragma once
# ifndef PIPELINE
# define PIPELINE

# include "pipeline/storage.h"
# include "pipeline/triggers.h"

int initializeTinyBCIPipeline(const float *frequencies, uint8_t channelCount, uint32_t sampleRate);
void cleanUpTinyBCIPipeline(void);

int startTinyBCIPipeline(void);
int startTinyBCIPipelineInState(TBCI_State initialState);
int updateTinyBCIPipeline(void);
int stopTinyBCIPipeline(void);

int reportAndReturnPipelineStatus(TBCI_Status status, const char *actionLabel);

typedef struct {
    int16_t predictedLabel;
    uint16_t targetLabel;
    float confidence;
    float confidences[N_FREQS];
} TinyBCIInference;

bool tryGetTinyBCIInference(TinyBCIInference *out);
void pushEEGSampleToTinyBCIPipeline(float *samples, uint32_t index);

# endif