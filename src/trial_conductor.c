# include "trial_conductor.h"
# include "microsecond_timer.h"

static void (*trialEndCallback)() = NULL;
static void (*trialStartCallback)(uint16_t) = NULL;

static MicrosecondTimer trialDurationTimer;
static MicrosecondTimer breakTimer;

static uint16_t targetCount = 0;
static uint16_t target = 0;

enum State { BREAK, TRIAL };
static uint16_t state = BREAK;


void initializeTrialConductor(
    uint16_t pTargetCount,
    float trialDuration, float breakDuration,
    void (*onTrialStart)(uint16_t), void (*onTrialEnd)())
{
    targetCount = pTargetCount;
    target = pTargetCount - 1;

    uint64_t microsecondsPerTrial = (uint64_t)(trialDuration * 1000000);
    trialDurationTimer = createMicrosecondTimer(trialDuration);
    breakTimer = createMicrosecondTimer(breakDuration);

    trialStartCallback = onTrialStart;
    trialEndCallback = onTrialEnd;
}

// ---

void startTrial()
{
    target = (target + 1) % targetCount;
    pushTrigger(target);
    state = TRIAL;

    if (trialStartCallback != NULL) trialStartCallback(target);
    resetMicrosecondTimer(&trialDurationTimer);
}

void endTrial()
{
    pushTrialEndCode();
    state = BREAK;

    if (trialEndCallback != NULL) trialEndCallback();
    resetMicrosecondTimer(&breakTimer);
}

void updateTrialConductor()
{
    if (state == BREAK)
    {
        if (checkMicrosecondTimer(&breakTimer)) startTrial();
    }
    else
    {
        if (checkMicrosecondTimer(&trialDurationTimer)) endTrial();
    }
}