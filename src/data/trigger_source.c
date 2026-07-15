# include "data/trigger_source.h"
# include "microsecond_timer.h"

void pushTrigger(uint16_t value)
{
    uint64_t timestamp = getCurrentMicrosecondTimestamp();
    printf("%lu | Pushing trigger : %u\n", timestamp, value);

    TBCI_Trigger trigger = {
        .timestamp_us = timestamp,
        .code = value,
        .type = TBCI_TRIGGER_DATA
    };
    in_push_trigger(&tbciInputs, &trigger, &tbciContext);
}

void pushTrialEndCode()
{
    pushTrigger(TRIAL_END_CODE);
}