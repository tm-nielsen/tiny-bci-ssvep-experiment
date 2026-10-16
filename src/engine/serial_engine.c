# include "engine/serial_engine.h"
# include "serial/data_source.h"
# include "cli/eeg_source_selection.h"   // promptSerialPortSelection()
# include "microsecond_timer.h"

# include <stdio.h>
# include <string.h>

#include "serial/bci_link.h"

# define SILENCE_TIMEOUT_US 3000000   // engine sends STATUS every second
# define INFERENCE_QUEUE_SIZE 16

static char portName[256] = { 0 };
static SerialDataSource source;
static bool sourceOpen = false;

static BCIEngineState engineState = BCI_ENGINE_BOOTING;
static uint64_t lastFrameUs = 0;
static uint8_t txSequence = 0;
static uint32_t rejectedFrames = 0;

typedef struct { TinyBCIInference inference; uint64_t timestampUs; } QueuedInference;
static QueuedInference inferenceQueue[INFERENCE_QUEUE_SIZE];
static uint8_t queueHead = 0, queueTail = 0;

// ---

static void sendMessage(BCILinkMessage message)
{
    if (!sourceOpen) return;
    message.sequence = txSequence++;
    uint8_t frame[BCI_LINK_FRAME_SIZE];
    encodeBCILinkFrame(&message, frame);
    serialWrite(&source.handle, frame, BCI_LINK_FRAME_SIZE);
}

static void queueInference(const BCILinkInference *received)
{
    QueuedInference *slot = &inferenceQueue[queueHead];
    memset(slot, 0, sizeof(*slot));

    // Wire labels are 1-based (as in the CSV); TinyBCIInference is 0-based.
    slot->inference.predictedLabel = (uint16_t)(received->predictedLabel - 1);
    slot->inference.targetLabel = (uint16_t)(received->trueLabel - 1);
    slot->inference.confidence = received->confidence;
    for (uint8_t i = 0; i < received->classCount && i < N_FREQS; i++)
        slot->inference.confidences[i] = received->probabilities[i];
    slot->timestampUs = received->timestampUs;

    queueHead = (uint8_t)((queueHead + 1) % INFERENCE_QUEUE_SIZE);
    if (queueHead == queueTail) queueTail = (uint8_t)((queueTail + 1) % INFERENCE_QUEUE_SIZE); // drop oldest
}

static void onFrame(SerialFrame frame)
{
    BCILinkMessage message;
    if (!decodeBCILinkFrame(frame.buffer, frame.size, &message)) { rejectedFrames++; return; }
    lastFrameUs = getCurrentMicrosecondTimestamp();

    switch (message.type)
    {
        case BCI_LINK_STATUS:
        {
            BCILinkStatus status;
            readBCILinkStatus(&message, &status);
            engineState = (BCIEngineState)status.state;
            break;
        }
        case BCI_LINK_INFERENCE:
        {
            BCILinkInference inference;
            readBCILinkInference(&message, &inference);
            queueInference(&inference);
            break;
        }
        default: // EEG, trigger echoes, logs: the collector records those
            break;
    }
}

// ---

void runSerialEnginePortSelection(void)
{
    printf("Select the engine's link port (TinyBCI Link A)\n");
    snprintf(portName, sizeof(portName), "%s", promptSerialPortSelection());
}

bool openSerialEngine(void)
{
    source = createSerialDataSource(
        BCI_LINK_FRAME_SIZE,
        MAKE_OXFF_TERMINATED_BYTE_ARRAY(BCI_LINK_START_BYTE0, BCI_LINK_START_BYTE1),
        MAKE_OXFF_TERMINATED_BYTE_ARRAY(BCI_LINK_STOP_BYTE0, BCI_LINK_STOP_BYTE1)
    );
    if (openSerialDataSource(&source, portName, 0) != EXIT_SUCCESS)
    {
        fprintf(stderr, "Could not open engine port '%s'\n", portName);
        return false;
    }
    setSerialDataSourceCallback(&source, onFrame);
    sourceOpen = true;
    lastFrameUs = getCurrentMicrosecondTimestamp();

    sendMessage(makeBCILinkHello((BCILinkHello){ BCI_LINK_PROTOCOL_VERSION, BCI_LINK_ROLE_PRESENTER }));
    return true;
}

void updateSerialEngine(void)
{
    if (sourceOpen) updateSerialDataSource(&source);
}

void closeSerialEngine(void)
{
    if (!sourceOpen) return;
    closeSerialDataSource(&source);
    sourceOpen = false;
}

bool isSerialEngineConnected(void)
{
    if (!sourceOpen || !isSerialDataSourceConnected(&source)) return false;
    return getCurrentMicrosecondTimestamp() - lastFrameUs < SILENCE_TIMEOUT_US;
}

bool isSerialEngineReady(void)
{
    return isSerialEngineConnected() && engineState == BCI_ENGINE_READY;
}

BCIEngineState getSerialEngineState(void) { return engineState; }

const char *getSerialEngineStateName(void)
{
    if (!isSerialEngineConnected()) return "Waiting for BCI Engine...";
    switch (engineState)
    {
        case BCI_ENGINE_BOOTING: return "BCI Engine: booting...";
        case BCI_ENGINE_CONNECTING_HEADSET: return "BCI Engine: connecting to headset...";
        case BCI_ENGINE_STABILIZING: return "BCI Engine: filters settling...";
        case BCI_ENGINE_READY: return "BCI Engine: ready";
        case BCI_ENGINE_FAULT: return "BCI Engine: FAULT (see its debug port)";
    }
    return "BCI Engine: unknown state";
}

void pushSerialEngineTrigger(uint16_t code)
{
    sendMessage(makeBCILinkTrigger((BCILinkTrigger){ code }));
}

bool pollSerialEngineInference(TinyBCIInference *inference, uint64_t *engineTimestampUs)
{
    if (queueTail == queueHead) return false;
    *inference = inferenceQueue[queueTail].inference;
    *engineTimestampUs = inferenceQueue[queueTail].timestampUs;
    queueTail = (uint8_t)((queueTail + 1) % INFERENCE_QUEUE_SIZE);
    return true;
}