# pragma once
# ifndef PROGRAM_FRAGMENTS
# define PROGRAM_FRAGMENTS

# include "pipeline.h"

void initializeTrialPresentation(
    void (*trialStartCallback)(uint16_t),
    void (*trialEndCallback)(uint16_t),
    void (*allTrialsCompletedCallback)(void)
);

void initializePipelineWithEEGSourceParameters(void);
void updatePipeline(void (*cleanUpMethod)(void));
void cleanUpEEGSourceAndPipeline(void);

void startUpdateThreadForSelectedEEGSource(void);
void cleanUpEEGSourceUpdateThread(void);
void lockEEGSourceMutex(void);
void unlockEEGSourceMutex(void);

void awaitFilterStabilization(void (*cleanUpMethod)(void));
void awaitConnection(
    bool (*predicate)(void),
    void (*updateMethod)(void),
    bool (*attemptMethod)(void)
);

void displayInference(TinyBCIInference inference, uint64_t timestamp);
void printInference(TinyBCIInference inference, uint64_t timestamp);

void displayMessageOrExit(const char *message, void (*cleanUpMethod)(void));
void closeIfPromptedTo(void (*cleanUpMethod)(void));

# endif