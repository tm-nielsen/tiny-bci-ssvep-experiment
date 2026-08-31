# include "pipeline.h"

int reportAndReturnPipelineStatus(TBCI_Status status, const char *actionLabel)
{
    if (status)
    {
        fprintf(stderr, "Failed to %s Tiny BCI Pipeline | code: %d\n", actionLabel, status);
    }
    return status;
}

// ---

void cleanUpTinyBCIPipeline(void)
{
    stopTinyBCIPipeline();
    deallocateDynamicStorage();
}

// ---

int startTinyBCIPipeline(void)
{
    TBCI_Status status = tbci_context_start(&tbciContext, TBCI_STATE_INFERENCE);
    return reportAndReturnPipelineStatus(status, "start");
}
int startTinyBCIPipelineInState(TBCI_State initialState)
{
    TBCI_Status status = tbci_context_start(&tbciContext, initialState);
    return reportAndReturnPipelineStatus(status, "start");
}

int updateTinyBCIPipeline(void)
{
    TBCI_Status status = tbci_context_tick(&tbciContext);
    return reportAndReturnPipelineStatus(status, "update");
}

int stopTinyBCIPipeline(void)
{
    TBCI_Status status = tbci_context_stop(&tbciContext);
    return reportAndReturnPipelineStatus(status, "stop");
}