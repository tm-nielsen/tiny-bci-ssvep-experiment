# include "pipeline.h"

int reportAndReturnPipelineStatus(TBCI_Status status, const char* actionLabel)
{
    if (status)
    {
        fprintf(stderr, "Failed to %s Tiny BCI Pipeline | code: %d\n", actionLabel, status);
    }
    return status;
}

// ---

void cleanUpTinyBCIPipeline()
{
    stopTinyBCIPipeline();
    deallocateDynamicStorage();
}

// ---

int startTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_start(&tbciContext, TBCI_STATE_INFERENCE);
    return reportAndReturnPipelineStatus(status, "start");
}
int startTinyBCIPipelineInState(TBCI_State initialState)
{
    TBCI_Status status = tbci_context_start(&tbciContext, initialState);
    return reportAndReturnPipelineStatus(status, "start");
}

int updateTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_tick(&tbciContext);
    return reportAndReturnPipelineStatus(status, "update");
}

int stopTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_stop(&tbciContext);
    return reportAndReturnPipelineStatus(status, "stop");
}