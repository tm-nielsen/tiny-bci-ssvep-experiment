# pragma once
# include "storage.h"

int initializeTinyBCIPipeline(const float *);

int startTinyBCIPipeline();
int startTinyBCIPipelineInState(TBCI_State);
int updateTinyBCIPipeline();
int stopTinyBCIPipeline();

bool tryGetTinyBCIInference(uint16_t *);