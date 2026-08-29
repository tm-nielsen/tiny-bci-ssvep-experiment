# pragma once
# include "pipeline.h"

void initializeInferenceLogger();
void notifyInferenceLoggerOfNewTarget(uint16_t target, uint64_t timestamp);
void logInference(TinyBCIInference inference, uint64_t timestamp);
void closeInferenceLogger();