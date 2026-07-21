# include "trial_conductor.h"
# include "microsecond_timer.h"

static void (*trialStartCallback)(uint16_t) = NULL;
static void (*trialEndCallback)(uint16_t) = NULL;

static MicrosecondTimer trialDurationTimer;
static MicrosecondTimer breakTimer;

static uint16_t targetCount = 0;
static uint16_t target = 0;

enum State { BREAK, TRIAL };
static uint16_t state = BREAK;


void initializeTrialConductor(
    uint16_t pTargetCount,
    float trialDuration, float breakDuration,
    void (*onTrialStart)(uint16_t), void (*onTrialEnd)(uint16_t))
{
    targetCount = pTargetCount;

    uint64_t microsecondsPerTrial = (uint64_t)(trialDuration * 1000000);
    trialDurationTimer = createMicrosecondTimer(trialDuration);
    breakTimer = createMicrosecondTimer(breakDuration);

    trialStartCallback = onTrialStart;
    trialEndCallback = onTrialEnd;
}

uint16_t getTarget()
{
    return target + 1;
}
// ---

void startTrial()
{
    state = TRIAL;
    if (trialStartCallback != NULL) trialStartCallback(target);
    resetMicrosecondTimer(&trialDurationTimer);
}

void endTrial()
{
    state = BREAK;
    target = (target + 1) % targetCount;
    if (trialEndCallback != NULL) trialEndCallback(target);
    resetMicrosecondTimer(&breakTimer);
}


void updateTrialConductor()
{
    switch (state)
    {
        case BREAK:
            if (checkMicrosecondTimer(&breakTimer)) startTrial();
            break;
        case TRIAL:
            if (checkMicrosecondTimer(&trialDurationTimer)) endTrial();
            break;
        default: ;
    }
}

void resetTrialConductorTimers(void)
{
    state  = BREAK;
    target = 0;
    resetMicrosecondTimer(&breakTimer);
}