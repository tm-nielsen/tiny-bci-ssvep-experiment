# include "data/lsl_inference_channel.h"
# include "lsl_c.h"
# include <stdio.h>
# include <stdlib.h>

// Sample layout (all channels as double64, since a float32 mantissa can't
// hold a microsecond timestamp without losing precision):
//   [0]              predictedLabel
//   [1]              confidence
//   [2 .. 1+N_FREQS] confidences[N_FREQS]
//   [2+N_FREQS]      targetLabel (ground truth from the trial conductor)
//   [3+N_FREQS]      timestampUs
// --- Runtime side (publisher) ---

static lsl_outlet outlet = NULL;

int openLslInferenceOutlet(const char *streamName)
{
    lsl_streaminfo outletInfo = lsl_create_streaminfo(
        streamName,
        INFERENCE_STREAM_TYPE,
        INFERENCE_CHANNEL_COUNT, LSL_IRREGULAR_RATE,
        cft_double64,
        INFERENCE_STREAM_SOURCE_ID
    );
    if (outletInfo == NULL)
    {
        printf("Failed to create LSL inference stream info\n");
        return -1;
    }

    outlet = lsl_create_outlet(outletInfo, 0, 360);
    lsl_destroy_streaminfo(outletInfo);

    if (outlet == NULL)
    {
        printf("Failed to open LSL inference outlet\n");
        return -1;
    }

    return 0;
}

void pushLslInference(const TinyBCIInference *inference, uint64_t timestampUs, uint16_t targetLabel)
{
    if (outlet == NULL)
    {
        printf("Attempting to start LSL inference outlet\n");
        if (openLslInferenceOutlet(INFERENCE_STREAM_NAME_DEFAULT) != 0) return;
    }

    double sample[INFERENCE_CHANNEL_COUNT];
    sample[0] = (double)inference->predictedLabel;
    sample[1] = (double)inference->confidence;
    for (int i = 0; i < N_FREQS; i++)
    {
        sample[2 + i] = (double)inference->confidences[i];
    }
    sample[2 + N_FREQS] = (double)targetLabel;
    sample[3 + N_FREQS] = (double)timestampUs;

    int32_t pushError = lsl_push_sample_d(outlet, sample);
    if (pushError != lsl_no_error)
    {
        printf("Error pushing inference to LSL stream\n");
    }
}

void closeLslInferenceOutlet(void)
{
    if (outlet != NULL)
    {
        lsl_destroy_outlet(outlet);
        outlet = NULL;
    }
}

// --- Presenter side (subscriber) ---

static lsl_inlet inlet = NULL;

int openLslInferenceInlet(const char *streamName)
{
    lsl_streaminfo resolved[1];
    int resolvedCount = lsl_resolve_byprop(resolved, 1, "name", streamName, 1, 0.2);

    if (resolvedCount < 1)
    {
        printf("resolve failed for '%s', count=%d\n", streamName, resolvedCount); // <-- add here
        return -1;
    }

    inlet = lsl_create_inlet(resolved[0], 360, LSL_NO_PREFERENCE, 1);
    lsl_destroy_streaminfo(resolved[0]);

    if (inlet == NULL)
    {
        printf("Failed to open LSL inference inlet\n");
        return -1;
    }

    lsl_open_stream(inlet, LSL_FOREVER, NULL);
    return 0;
}

int pollLslInference(TinyBCIInference *out, uint64_t *timestampUs, uint16_t *targetLabel)
{
    if (inlet == NULL) return 0;

    double sample[INFERENCE_CHANNEL_COUNT];
    double sampleTimestamp = lsl_pull_sample_d(inlet, sample, INFERENCE_CHANNEL_COUNT, 0.0, NULL);

    if (sampleTimestamp == 0.0) return 0; // nothing currently available, non-blocking

    out->predictedLabel = (uint16_t)sample[0];
    out->confidence = (float)sample[1];
    for (int i = 0; i < N_FREQS; i++)
    {
        out->confidences[i] = (float)sample[2 + i];
    }
    *targetLabel = (uint16_t)sample[2 + N_FREQS];
    *timestampUs = (uint64_t)sample[3 + N_FREQS];

    return 1;
}

void closeLslInferenceInlet(void)
{
    if (inlet != NULL)
    {
        lsl_destroy_inlet(inlet);
        inlet = NULL;
    }
}

bool isLslInferenceConnected(void)
{
    return inlet != NULL;
}