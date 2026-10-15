# include "serial/bci_link.h"
# include <string.h>

_Static_assert(BCI_LINK_FRAME_SIZE == 56, "frame size changed: update every receiver");
_Static_assert(16 + 4 * BCI_LINK_EEG_CHANNELS <= BCI_LINK_PAYLOAD_SIZE, "EEG payload overflow");
_Static_assert(24 + 4 * BCI_LINK_MAX_CLASSES <= BCI_LINK_PAYLOAD_SIZE, "inference payload overflow");

# define HEADER_SIZE 4                          // start bytes, type, sequence
# define CRC_OFFSET  (HEADER_SIZE + BCI_LINK_PAYLOAD_SIZE)

// --- little-endian field access

static void putU16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void putU32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void putU64(uint8_t *p, uint64_t v) { for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i)); }
static void putF32(uint8_t *p, float v) { uint32_t u; memcpy(&u, &v, 4); putU32(p, u); }

static uint16_t getU16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t getU32(const uint8_t *p)
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= (uint32_t)p[i] << (8 * i);
    return v;
}
static uint64_t getU64(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)p[i] << (8 * i);
    return v;
}
static float getF32(const uint8_t *p) { uint32_t u = getU32(p); float v; memcpy(&v, &u, 4); return v; }

static uint16_t crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (int bit = 0; bit < 8; bit++)
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    }
    return crc;
}

// --- framing

size_t encodeBCILinkFrame(const BCILinkMessage *message, uint8_t frame[BCI_LINK_FRAME_SIZE])
{
    frame[0] = BCI_LINK_START_BYTE0;
    frame[1] = BCI_LINK_START_BYTE1;
    frame[2] = message->type;
    frame[3] = message->sequence;
    memcpy(&frame[HEADER_SIZE], message->payload, BCI_LINK_PAYLOAD_SIZE);
    putU16(&frame[CRC_OFFSET], crc16(&frame[2], CRC_OFFSET - 2));
    frame[CRC_OFFSET + 2] = BCI_LINK_STOP_BYTE0;
    frame[CRC_OFFSET + 3] = BCI_LINK_STOP_BYTE1;
    return BCI_LINK_FRAME_SIZE;
}

bool decodeBCILinkFrame(const uint8_t *frame, size_t size, BCILinkMessage *message)
{
    if (size != BCI_LINK_FRAME_SIZE) return false;
    if (frame[0] != BCI_LINK_START_BYTE0 || frame[1] != BCI_LINK_START_BYTE1) return false;
    if (frame[size - 2] != BCI_LINK_STOP_BYTE0 || frame[size - 1] != BCI_LINK_STOP_BYTE1) return false;
    if (getU16(&frame[CRC_OFFSET]) != crc16(&frame[2], CRC_OFFSET - 2)) return false;

    message->type = frame[2];
    message->sequence = frame[3];
    memcpy(message->payload, &frame[HEADER_SIZE], BCI_LINK_PAYLOAD_SIZE);
    return true;
}

void resetBCILinkParser(BCILinkParser *parser)
{
    parser->count = 0;
    parser->rejectedFrames = 0;
}

// Drop the first byte and slide to the next possible start byte.
static void resynchronize(BCILinkParser *parser)
{
    size_t next = 1;
    while (next < parser->count && parser->buffer[next] != BCI_LINK_START_BYTE0) next++;
    parser->count -= next;
    memmove(parser->buffer, &parser->buffer[next], parser->count);
}

bool pushBCILinkParserByte(BCILinkParser *parser, uint8_t byte, BCILinkMessage *message)
{
    parser->buffer[parser->count++] = byte;

    while (parser->count > 0)
    {
        if (parser->buffer[0] != BCI_LINK_START_BYTE0) { resynchronize(parser); continue; }
        if (parser->count >= 2 && parser->buffer[1] != BCI_LINK_START_BYTE1) { resynchronize(parser); continue; }
        if (parser->count < BCI_LINK_FRAME_SIZE) return false;

        if (decodeBCILinkFrame(parser->buffer, BCI_LINK_FRAME_SIZE, message))
        {
            parser->count = 0;
            return true;
        }
        parser->rejectedFrames++;
        resynchronize(parser);
    }
    return false;
}

// --- builders

static BCILinkMessage emptyMessage(BCILinkMessageType type)
{
    BCILinkMessage message;
    memset(&message, 0, sizeof(message));
    message.type = (uint8_t)type;
    return message;
}

BCILinkMessage makeBCILinkHello(BCILinkHello v)
{
    BCILinkMessage m = emptyMessage(BCI_LINK_HELLO);
    putU16(&m.payload[0], v.protocolVersion);
    m.payload[2] = v.role;
    return m;
}

BCILinkMessage makeBCILinkTrigger(BCILinkTrigger v)
{
    BCILinkMessage m = emptyMessage(BCI_LINK_TRIGGER);
    putU16(&m.payload[0], v.code);
    return m;
}

static BCILinkMessage makeTimeSync(BCILinkMessageType type, BCILinkTimeSync v)
{
    BCILinkMessage m = emptyMessage(type);
    putU64(&m.payload[0], v.hostTimeUs);
    putU64(&m.payload[8], v.engineTimeUs);
    return m;
}

BCILinkMessage makeBCILinkPing(BCILinkTimeSync v) { return makeTimeSync(BCI_LINK_PING, v); }
BCILinkMessage makeBCILinkPong(BCILinkTimeSync v) { return makeTimeSync(BCI_LINK_PONG, v); }

BCILinkMessage makeBCILinkStatus(BCILinkStatus v)
{
    BCILinkMessage m = emptyMessage(BCI_LINK_STATUS);
    m.payload[0] = v.state;
    m.payload[1] = v.clientCount;
    putU32(&m.payload[2], v.samplesReceived);
    putU32(&m.payload[6], v.framesDropped);
    putU32(&m.payload[10], v.maxTickUs);
    putU64(&m.payload[14], v.engineTimeUs);
    return m;
}

BCILinkMessage makeBCILinkEEG(const BCILinkEEG *v)
{
    BCILinkMessage m = emptyMessage(BCI_LINK_EEG);
    uint8_t count = v->channelCount > BCI_LINK_EEG_CHANNELS ? BCI_LINK_EEG_CHANNELS : v->channelCount;
    putU32(&m.payload[0], v->sampleIndex);
    putU64(&m.payload[4], v->timestampUs);
    putU16(&m.payload[12], v->triggerValue);
    m.payload[14] = v->firstChannel;
    m.payload[15] = count;
    for (uint8_t i = 0; i < count; i++) putF32(&m.payload[16 + 4 * i], v->channels[i]);
    return m;
}

BCILinkMessage makeBCILinkInference(const BCILinkInference *v)
{
    BCILinkMessage m = emptyMessage(BCI_LINK_INFERENCE);
    uint8_t count = v->classCount > BCI_LINK_MAX_CLASSES ? BCI_LINK_MAX_CLASSES : v->classCount;
    putU64(&m.payload[0], v->timestampUs);
    putU32(&m.payload[8], v->deltaTimeUs);
    putU16(&m.payload[12], v->trueLabel);
    putU16(&m.payload[14], v->predictedLabel);
    putU16(&m.payload[16], v->roundNumber);
    m.payload[18] = count;
    putF32(&m.payload[20], v->confidence);
    for (uint8_t i = 0; i < count; i++) putF32(&m.payload[24 + 4 * i], v->probabilities[i]);
    return m;
}

BCILinkMessage makeBCILinkTriggerEcho(BCILinkTriggerEcho v)
{
    BCILinkMessage m = emptyMessage(BCI_LINK_TRIGGER_ECHO);
    putU16(&m.payload[0], v.code);
    putU64(&m.payload[2], v.timestampUs);
    return m;
}

BCILinkMessage makeBCILinkLog(const char *text)
{
    BCILinkMessage m = emptyMessage(BCI_LINK_LOG);
    size_t length = strlen(text);
    if (length > BCI_LINK_LOG_LENGTH) length = BCI_LINK_LOG_LENGTH;
    m.payload[0] = (uint8_t)length;
    memcpy(&m.payload[1], text, length);
    return m;
}

// --- readers

bool readBCILinkHello(const BCILinkMessage *m, BCILinkHello *out)
{
    if (m->type != BCI_LINK_HELLO) return false;
    out->protocolVersion = getU16(&m->payload[0]);
    out->role = m->payload[2];
    return true;
}

bool readBCILinkTrigger(const BCILinkMessage *m, BCILinkTrigger *out)
{
    if (m->type != BCI_LINK_TRIGGER) return false;
    out->code = getU16(&m->payload[0]);
    return true;
}

bool readBCILinkTimeSync(const BCILinkMessage *m, BCILinkTimeSync *out)
{
    if (m->type != BCI_LINK_PING && m->type != BCI_LINK_PONG) return false;
    out->hostTimeUs = getU64(&m->payload[0]);
    out->engineTimeUs = getU64(&m->payload[8]);
    return true;
}

bool readBCILinkStatus(const BCILinkMessage *m, BCILinkStatus *out)
{
    if (m->type != BCI_LINK_STATUS) return false;
    out->state = m->payload[0];
    out->clientCount = m->payload[1];
    out->samplesReceived = getU32(&m->payload[2]);
    out->framesDropped = getU32(&m->payload[6]);
    out->maxTickUs = getU32(&m->payload[10]);
    out->engineTimeUs = getU64(&m->payload[14]);
    return true;
}

bool readBCILinkEEG(const BCILinkMessage *m, BCILinkEEG *out)
{
    if (m->type != BCI_LINK_EEG) return false;
    memset(out, 0, sizeof(*out));
    out->sampleIndex = getU32(&m->payload[0]);
    out->timestampUs = getU64(&m->payload[4]);
    out->triggerValue = getU16(&m->payload[12]);
    out->firstChannel = m->payload[14];
    out->channelCount = m->payload[15] > BCI_LINK_EEG_CHANNELS ? BCI_LINK_EEG_CHANNELS : m->payload[15];
    for (uint8_t i = 0; i < out->channelCount; i++) out->channels[i] = getF32(&m->payload[16 + 4 * i]);
    return true;
}

bool readBCILinkInference(const BCILinkMessage *m, BCILinkInference *out)
{
    if (m->type != BCI_LINK_INFERENCE) return false;
    memset(out, 0, sizeof(*out));
    out->timestampUs = getU64(&m->payload[0]);
    out->deltaTimeUs = getU32(&m->payload[8]);
    out->trueLabel = getU16(&m->payload[12]);
    out->predictedLabel = getU16(&m->payload[14]);
    out->roundNumber = getU16(&m->payload[16]);
    out->classCount = m->payload[18] > BCI_LINK_MAX_CLASSES ? BCI_LINK_MAX_CLASSES : m->payload[18];
    out->confidence = getF32(&m->payload[20]);
    for (uint8_t i = 0; i < out->classCount; i++) out->probabilities[i] = getF32(&m->payload[24 + 4 * i]);
    return true;
}

bool readBCILinkTriggerEcho(const BCILinkMessage *m, BCILinkTriggerEcho *out)
{
    if (m->type != BCI_LINK_TRIGGER_ECHO) return false;
    out->code = getU16(&m->payload[0]);
    out->timestampUs = getU64(&m->payload[2]);
    return true;
}

bool readBCILinkLog(const BCILinkMessage *m, BCILinkLog *out)
{
    if (m->type != BCI_LINK_LOG) return false;
    out->length = m->payload[0] > BCI_LINK_LOG_LENGTH ? BCI_LINK_LOG_LENGTH : m->payload[0];
    memcpy(out->text, &m->payload[1], out->length);
    out->text[out->length] = '\0';
    return true;
}