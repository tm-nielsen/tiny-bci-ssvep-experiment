# pragma once

void initializeTrialConductor(uint16_t, uint16_t, float, float);

void setTrialStartCallback(void (*onTrialStart)(uint16_t));
void setTrialEndCallback(void (*onTrialEnd)(uint16_t));
void setStimulusRoundCompleteCallback(void (*onStimulusRoundCompleted));
void setAllTrialsCompletedCallback(void (*onAllTrialsCompleted));

void updateTrialConductor(void);
void resetTrialConductor(void);
void cleanUpTrialConductor(void);

void seedRandomizationWithCurrentTime(void);
void randomizeTargetOrder(uint16_t *targets, uint16_t targetCount);