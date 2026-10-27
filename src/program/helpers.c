# include "program/helpers.h"
# include "program/constants.h"

# include "pipeline.h"
# include "microsecond_timer.h"
# include "trial_conductor.h"

void awaitConnection(
    bool (*predicate)(void),
    void (*updateMethod)(void),
    bool (*attemptMethod)(void)
)
{
    MicrosecondTimer connectionAttemptTimer =
        createMicrosecondTimer(CONNECTION_ATTEMPT_INTERVAL);
    resetMicrosecondTimer(&connectionAttemptTimer);

    while(!predicate())
    {
        while (!checkMicrosecondTimer(&connectionAttemptTimer))
        {
            updateMethod();
        }
        attemptMethod();
    }
}

void printInference(TinyBCIInference inference, uint64_t timestamp)
{
    printf(
        "%" PRIu64 " | Output received: %d (%.0f%% confidence) [",
        timestamp, inference.predictedLabel, inference.confidence * 100
    );
    for (int i = 0; i < N_FREQS; i++) printf(" %.2f", inference.confidences[i]);
    printf(" ]\n");
}

// ---

void seedRandomizationWithCurrentTime(void)
{
    srand(getCurrentMicrosecondTimestamp());
}

void randomizeTargetOrder(uint16_t *targets, uint16_t targetCount)
{
    for (uint16_t i = 0; i < targetCount; i++) targets[i] = i;

    for (uint16_t remaining = targetCount; remaining > 1; remaining--)
    {
        uint16_t selected = (uint16_t)(rand() % remaining);
        uint16_t last = targets[remaining - 1];
        targets[remaining - 1] = targets[selected];
        targets[selected] = last;
    }
}