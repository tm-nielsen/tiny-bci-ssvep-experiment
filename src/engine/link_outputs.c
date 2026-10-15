# include "engine/link_outputs.h"
# include "engine/link_hub.h"
# include "inference_logger.h"
# include "microsecond_timer.h"

_Static_assert(N_FREQS <= BCI_LINK_MAX_CLASSES, "raise BCI_LINK_MAX_CLASSES");

static uint32_t eegSampleIndex = 0;   // same counter as the raw CSV's sample_idx

// Raw logger tap: called once per frame, in order, inside the pipeline tick.
static void onRawFrame(
    const float *samples, size_t channelCount,
    const TBCI_Frame *frame, uint16_t triggerValue,
    void *userData
) {
    (void)userData;

    // Samples with more than BCI_LINK_EEG_CHANNELS channels span several frames.
    for (size_t first = 0; first < channelCount; first += BCI_LINK_EEG_CHANNELS)
    {
        size_t count = channelCount - first;
        if (count > BCI_LINK_EEG_CHANNELS) count = BCI_LINK_EEG_CHANNELS;

        BCILinkEEG eeg = {
            .sampleIndex = eegSampleIndex,
            .timestampUs = frame->timestamp_us,
            .triggerValue = triggerValue,
            .firstChannel = (uint8_t)first,
            .channelCount = (uint8_t)count,
        };
        for (size_t i = 0; i < count; i++) eeg.channels[i] = samples[first + i];
        broadcastLinkMessage(makeBCILinkEEG(&eeg));
    }
    eegSampleIndex++;
}

void installEEGLinkTap(void)
{
    eegSampleIndex = 0;
    tbciContext.core_node.raw_out.on_frame = &onRawFrame;
    tbciContext.core_node.raw_out.user_data = NULL;

    // The node is disabled when CSV logging is off. Enabling it lets the
    // pipeline call it every tick; with no file open it only feeds the tap.
    tbciContext.core_node.raw_out.base.enabled = true;
}

uint32_t getEEGFramesSent(void) { return eegSampleIndex; }

void sendInferenceToLink(TinyBCIInference inference, uint64_t timestamp)
{
    InferenceRecord record = makeInferenceRecord(inference, timestamp);

    BCILinkInference message = {
        .timestampUs = record.timestamp_us,
        .deltaTimeUs = (uint32_t)record.delta_time_us,
        .trueLabel = record.true_label,
        .predictedLabel = record.predicted_label,
        .roundNumber = record.round_number,
        .classCount = N_FREQS,
        .confidence = record.confidence,
    };
    for (int i = 0; i < N_FREQS; i++) message.probabilities[i] = record.probabilities[i];
    broadcastLinkMessage(makeBCILinkInference(&message));
}

void sendTriggerEchoToLink(uint16_t code, uint64_t timestamp)
{
    broadcastLinkMessage(makeBCILinkTriggerEcho((BCILinkTriggerEcho){ code, timestamp }));
}

void sendStatusToLink(BCIEngineState state, uint32_t maxTickUs)
{
    BCILinkStatus status = {
        .state = (uint8_t)state,
        .clientCount = getLinkClientCount(),
        .samplesReceived = eegSampleIndex,
        .framesDropped = getLinkFramesDropped(),
        .maxTickUs = maxTickUs,
        .engineTimeUs = getCurrentMicrosecondTimestamp(),
    };
    broadcastLinkMessage(makeBCILinkStatus(status));
}