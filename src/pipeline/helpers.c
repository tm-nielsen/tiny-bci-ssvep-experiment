# include "pipeline.h"
# include "microsecond_timer.h"

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

void pushEEGSampleToTinyBCIPipelineWithCurrentTimestamp(float *samples, uint32_t index)
{
    uint64_t timestamp = getCurrentMicrosecondTimestamp();
    pushEEGSampleToTinyBCIPipelineWithTimestamp(samples, index, timestamp);
}

void pushEEGSampleToTinyBCIPipelineWithTimestamp(
    float *samples, uint32_t index,
    uint64_t microsecondTimestamp
)
{
    in_push_signal(&tbciInputs, samples, microsecondTimestamp, index);
}

// ---

static uint64_t EEGSampleAnchorTimestamp = 0;
static uint16_t EEGSampleAnchorIndex;

static uint64_t EEGSamplePeriod = 0;

void configureEEGTimestampAnchorSamplePeriod(uint32_t sampleRate)
{
    EEGSamplePeriod = 1000000 / sampleRate;
}

void renewEEGSampleTimestampAnchor(uint32_t nextSampleIndex)
{
    EEGSampleAnchorTimestamp = getCurrentMicrosecondTimestamp();
    EEGSampleAnchorIndex = nextSampleIndex;
}

void pushEEGSampleToTinyBCIPipelineWithAnchoredTimestamp(float *samples, uint32_t index)
{
    if (EEGSamplePeriod == 0)
    {
        fprintf(stderr, "Error: Sample Period unset - cannot use push sample using anchored timestamp\n");
        exit(EXIT_SUCCESS);
    }
    if (EEGSampleAnchorTimestamp == 0)
    {
        fprintf(stderr, "Error: Sample Timestamp Anchor unset - cannot push sample using anchored timestamp\n");
        exit(EXIT_SUCCESS);
    }

    uint32_t offset = index - EEGSampleAnchorIndex;
    if (index < EEGSampleAnchorIndex)
    {
        offset = index + UINT32_MAX - EEGSampleAnchorIndex;
    }

    uint64_t timestamp = EEGSampleAnchorTimestamp + offset * EEGSamplePeriod;
    pushEEGSampleToTinyBCIPipelineWithTimestamp(samples, index, timestamp);
}