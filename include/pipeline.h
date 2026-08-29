# pragma once
# ifndef PIPELINE
# define PIPELINE

# include "pipeline/storage.h"
# include "pipeline/triggers.h"

int initializeTinyBCIPipeline(const float *, uint8_t, uint32_t);
void cleanUpTinyBCIPipeline();

int startTinyBCIPipeline();
int startTinyBCIPipelineInState(TBCI_State);
int updateTinyBCIPipeline();
int stopTinyBCIPipeline();

int reportAndReturnPipelineStatus(TBCI_Status status, const char* actionLabel);

typedef struct {
    int16_t predictedLabel;
    uint16_t targetLabel;
    float confidence;
    float confidences[N_FREQS];
} TinyBCIInference;

bool tryGetTinyBCIInference(TinyBCIInference *out);
void pushEEGSampleToTinyBCIPipeline(float *samples, uint32_t index);

# endif