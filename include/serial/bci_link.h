# pragma once
# ifndef BCI_LINK
# define BCI_LINK

/*
 * Framed binary protocol between a TinyBCI engine (the Pico) and its clients
 * (presenter, collector) over USB serial or TCP. Platform-neutral: no SDK or
 * pipeline dependencies, so the C presenter compiles the same file.
 *
 *   [A5 5A] [type] [seq] [payload: 48 bytes] [crc16 LE] [0D 0A]   = 56 bytes
 *
 * CRC16-CCITT (poly 0x1021, init 0xFFFF) over type, seq and payload.
 * Multi-byte fields are little-endian, floats IEEE-754 binary32.
 * seq increments per frame sent, per sender; gaps mean lost frames.
 */

# include <stdint.h>
# include <stdbool.h>
# include <stddef.h>

# define BCI_LINK_PROTOCOL_VERSION 1

# define BCI_LINK_START_BYTE0 0xA5
# define BCI_LINK_START_BYTE1 0x5A
# define BCI_LINK_STOP_BYTE0  0x0D
# define BCI_LINK_STOP_BYTE1  0x0A

# define BCI_LINK_PAYLOAD_SIZE 48
# define BCI_LINK_FRAME_SIZE   (4 + BCI_LINK_PAYLOAD_SIZE + 4)   // 56

# define BCI_LINK_EEG_CHANNELS 8   // one EEG sample per frame
# define BCI_LINK_MAX_CLASSES  6   // must be >= N_FREQS
# define BCI_LINK_LOG_LENGTH   (BCI_LINK_PAYLOAD_SIZE - 1)

typedef enum
{
    // client -> engine
    BCI_LINK_HELLO        = 0x01,
    BCI_LINK_TRIGGER      = 0x02,
    BCI_LINK_PING         = 0x03,

    // engine -> clients
    BCI_LINK_STATUS       = 0x81,
    BCI_LINK_EEG          = 0x82,
    BCI_LINK_INFERENCE    = 0x83,
    BCI_LINK_TRIGGER_ECHO = 0x84,
    BCI_LINK_LOG          = 0x85,
    BCI_LINK_PONG         = 0x86,
} BCILinkMessageType;

typedef enum { BCI_LINK_ROLE_PRESENTER = 1, BCI_LINK_ROLE_COLLECTOR = 2 } BCILinkRole;

typedef enum
{
    BCI_ENGINE_BOOTING = 0,
    BCI_ENGINE_CONNECTING_HEADSET,
    BCI_ENGINE_STABILIZING,
    BCI_ENGINE_READY,
    BCI_ENGINE_FAULT,
} BCIEngineState;

typedef struct
{
    uint8_t type;
    uint8_t sequence;
    uint8_t payload[BCI_LINK_PAYLOAD_SIZE];
} BCILinkMessage;

// --- payloads (byte offsets in comments)

typedef struct { uint16_t protocolVersion; uint8_t role; } BCILinkHello;   // 0, 2
typedef struct { uint16_t code; } BCILinkTrigger;                          // 0
typedef struct { uint64_t hostTimeUs; uint64_t engineTimeUs; } BCILinkTimeSync; // 0, 8 (PING/PONG)

typedef struct                     // one raw EEG CSV row (or part of it)
{
    uint32_t sampleIndex;          // 0
    uint64_t timestampUs;          // 4
    uint16_t triggerValue;         // 12
    uint8_t  firstChannel;         // 14  index of channels[0] within the sample
    uint8_t  channelCount;         // 15  valid entries in channels[] (1..8)
    float channels[BCI_LINK_EEG_CHANNELS]; // 16
} BCILinkEEG;

typedef struct                     // one inference CSV row
{
    uint64_t timestampUs;          // 0
    uint32_t deltaTimeUs;          // 8
    uint16_t trueLabel;            // 12  1-based
    uint16_t predictedLabel;       // 14  1-based
    uint16_t roundNumber;          // 16
    uint8_t classCount;            // 18  (19 reserved)
    float confidence;              // 20
    float probabilities[BCI_LINK_MAX_CLASSES]; // 24
} BCILinkInference;

typedef struct { uint16_t code; uint64_t timestampUs; } BCILinkTriggerEcho; // 0, 2

typedef struct
{
    uint8_t state;                 // 0  BCIEngineState
    uint8_t clientCount;           // 1
    uint32_t samplesReceived;      // 2
    uint32_t framesDropped;        // 6  outgoing frames that didn't fit
    uint32_t maxTickUs;            // 10 worst pipeline tick since last status
    uint64_t engineTimeUs;         // 14
} BCILinkStatus;

typedef struct { uint8_t length; char text[BCI_LINK_LOG_LENGTH]; } BCILinkLog; // 0, 1

// --- framing

size_t encodeBCILinkFrame(const BCILinkMessage *message, uint8_t frame[BCI_LINK_FRAME_SIZE]);
bool decodeBCILinkFrame(const uint8_t *frame, size_t size, BCILinkMessage *message);

// Stream parser: feed bytes one at a time, resyncs after corruption.
typedef struct
{
    uint8_t buffer[BCI_LINK_FRAME_SIZE];
    size_t count;
    uint32_t rejectedFrames;
} BCILinkParser;

void resetBCILinkParser(BCILinkParser *parser);
bool pushBCILinkParserByte(BCILinkParser *parser, uint8_t byte, BCILinkMessage *message);

// --- typed builders (sequence left 0; the sender stamps it)

BCILinkMessage makeBCILinkHello(BCILinkHello value);
BCILinkMessage makeBCILinkTrigger(BCILinkTrigger value);
BCILinkMessage makeBCILinkPing(BCILinkTimeSync value);
BCILinkMessage makeBCILinkPong(BCILinkTimeSync value);
BCILinkMessage makeBCILinkStatus(BCILinkStatus value);
BCILinkMessage makeBCILinkEEG(const BCILinkEEG *value);
BCILinkMessage makeBCILinkInference(const BCILinkInference *value);
BCILinkMessage makeBCILinkTriggerEcho(BCILinkTriggerEcho value);
BCILinkMessage makeBCILinkLog(const char *text);

// --- typed readers (false if the type doesn't match)

bool readBCILinkHello(const BCILinkMessage *m, BCILinkHello *out);
bool readBCILinkTrigger(const BCILinkMessage *m, BCILinkTrigger *out);
bool readBCILinkTimeSync(const BCILinkMessage *m, BCILinkTimeSync *out);
bool readBCILinkStatus(const BCILinkMessage *m, BCILinkStatus *out);
bool readBCILinkEEG(const BCILinkMessage *m, BCILinkEEG *out);
bool readBCILinkInference(const BCILinkMessage *m, BCILinkInference *out);
bool readBCILinkTriggerEcho(const BCILinkMessage *m, BCILinkTriggerEcho *out);
bool readBCILinkLog(const BCILinkMessage *m, BCILinkLog *out);

# endif