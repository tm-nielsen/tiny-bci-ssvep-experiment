# include "data/trigger_source.h"
# include "microsecond_timer.h"

static uint16_t triggerValue = 1;
static MicrosecondTimer timer = { .interval = TRIGGER_INTERVAL_MS * 1000 };

static TrialPointer ongoingTrials = NULL;

void startTrial(uint64_t timestamp)
{
    printf("starting trial at %llu\n", timestamp);
    TBCI_Trigger trigger = {
        .timestamp_us = timestamp,
        .code = triggerValue,
        .type = TBCI_TRIGGER_DATA
    };
    in_push_trigger(&tbciInputs, &trigger, &tbciContext);

    TrialPointer newTrial = (TrialPointer)malloc(sizeof(struct Trial));
    newTrial->startTime = timestamp;
    newTrial->endTime = timestamp + TRIAL_DURATION_MS * 1000;
    newTrial->next = NULL;

    if (ongoingTrials == NULL)
    {
        ongoingTrials = newTrial;
    }
    else
    {
        TrialPointer lastTrial = ongoingTrials;
        while (lastTrial-> next != NULL) 
        {
            lastTrial = lastTrial->next;
        }
        lastTrial->next = newTrial;
    }
}

void endMostRecentTrial()
{
    if (ongoingTrials == NULL)
    {
        fprintf(stderr, "Error: attempted to end a trial when there are none\n");
        return;
    }

    TrialPointer endingTrial = ongoingTrials;
    ongoingTrials = ongoingTrials->next;

    printf("end trial started at %llu\n", endingTrial->startTime);

    TBCI_Trigger trigger = {
        .timestamp_us = endingTrial->endTime,
        .code = TRIAL_END_CODE,
        .type = TBCI_TRIGGER_DATA
    };
    in_push_trigger(&tbciInputs, &trigger, &tbciContext);
    free(endingTrial);
}

void resetTriggerSource(uint16_t value)
{
    resetMicrosecondTimer(&timer);
    setTriggerValue(value);
}

void updateTriggerSource()
{
    uint64_t now = getCurrentMicrosecondTimestamp();

    if (checkMicrosecondTimer(&timer))
    {
        startTrial(now);
    }

    if (ongoingTrials != NULL && now > ongoingTrials->endTime)
    {
        endMostRecentTrial();
    }
}

// ---

void setTriggerValue(uint16_t value)
{
    triggerValue = value;
}