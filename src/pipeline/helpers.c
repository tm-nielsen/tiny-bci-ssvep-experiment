# include "pipeline.h"
# include "microsecond_timer.h"
# include "tinycthread.h"

static mtx_t signalBufferMutex;

bool tryGetTinyBCIInference(TinyBCIInference *out)
{
    if (eq_is_empty(&outputQueue))
        return false;

    TBCI_Epoch epoch;
    eq_pop(&outputQueue, &epoch);

    *out = (TinyBCIInference)
    {
        .predictedLabel = epoch.predicted_label,
        .targetLabel = epoch.label,
        .confidence = epoch.confidence
    };
    for (int i = 0; i < N_FREQS; i++)
    {
        out->confidences[i] = epoch.samples[i];
    }

    return true;
}

void pushEEGSampleToTinyBCIPipeline(float *samples, uint32_t index)
{
    uint64_t timestamp = getCurrentMicrosecondTimestamp();
    pushEEGSampleToTinyBCIPipelineWithTimestamp(samples, index, timestamp);
}

void pushEEGSampleToTinyBCIPipelineWithTimestamp(
    float *samples, uint32_t index,
    uint64_t microsecondTimestamp
)
{
    lockSignalBufferMutex();
    in_push_signal(&tbciInputs, samples, microsecondTimestamp, index);
    unlockSignalBufferMutex();
}

void initializeSignalBufferMutex(void) { mtx_init(&signalBufferMutex, mtx_plain); }
void lockSignalBufferMutex(void) { mtx_lock(&signalBufferMutex); }
void unlockSignalBufferMutex(void) { mtx_unlock(&signalBufferMutex); }
void cleanUpSignalBufferMutex(void) { mtx_destroy(&signalBufferMutex); }