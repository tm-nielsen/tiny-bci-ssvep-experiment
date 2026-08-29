# include "serial/data_source.h"

uint16_t countStoredBytes(SerialDataSource *source)
{
    if (source->isFull) return source->bufferLength;
    uint16_t tip = source->tip;
    uint16_t tail = source->tail;
    while (tip < tail) tip += source->bufferLength;
    return tip - tail;
}

uint16_t countRemainingBufferSize(SerialDataSource *source)
{
    if (source->isFull) return 0;
    uint16_t tip = source->tip;
    uint16_t tail = source->tail;
    while (tail <= tip) tail += source->bufferLength;
    return tail - tip;
}

void discardBytes(SerialDataSource *source, uint16_t count)
{
    if (count == 0) return;
    uint16_t storedBytes = countStoredBytes(source);
    source->tail = (source->tail + count) % source->bufferLength;
    if (count >= storedBytes) source->tip = source->tail;
    source->isFull = false;
}

// ---

bool tailMatchesFrameStart(SerialDataSource *source)
{
    for (uint8_t i = 0; source->startBytes[i] != 0xFF; i++)
    {
        uint16_t cursor = (source->tail + i) % source->bufferLength;
        if (source->buffer[cursor] != source->startBytes[i]) return false;
    }
    return true;
}

bool tailMatchesFrameEnd(SerialDataSource *source)
{
    uint16_t endBytesTail = source->tail + source->frameSize - 1;

    for (uint8_t i = 0; source->endBytes[i] != 0xFF; i++)
    {
        uint16_t cursor = (endBytesTail - i) % source->bufferLength;
        if (source->buffer[cursor] != source->endBytes[i]) return false;
    }
    return true;
}

bool hasAFrameWorthOfData(SerialDataSource *source)
{
    return countStoredBytes(source) >= source->frameSize;
}

bool hasValidFrameAtTail(SerialDataSource *source)
{
    if (!hasAFrameWorthOfData(source)) return false;
    if (!tailMatchesFrameStart(source)) return false;
    return tailMatchesFrameEnd(source);
}

// ---
# ifndef MIN
#   define MIN(a, b) ((a) < (b) ? (a) : (b))
# endif

void readIntoBuffer(SerialDataSource *source)
{
    if (source->isFull) return;
    int bytesRead = serialRead(
        &(source->handle),
        source->buffer + source->tip,
        MIN(
            source->bufferLength - source->tip,
            countRemainingBufferSize(source)
        )
    );
    if (bytesRead > 0)
    {
        source->tip = (source->tip + bytesRead) % source->bufferLength;
        source->isFull = source->tip == source->tail;
    }
}

SerialFrame extractFrame(SerialDataSource *source)
{
    SerialFrame frame = createSerialFrame(source->frameSize);
    for (
        uint16_t frameIndex = 0;
        frameIndex < source->frameSize;
        frameIndex++
    ) {
        uint16_t bufferIndex = (source->tail + frameIndex) % source->bufferLength;
        frame.buffer[frameIndex] = source->buffer[bufferIndex];
    }
    discardBytes(source, frame.size);
    return frame;
}

void seekFrameStart(SerialDataSource *source)
{
    while (!tailMatchesFrameStart(source))
    {
        if (source->tail == source->tip && !source->isFull) return;
        discardBytes(source, 1);
    }
}

void updateSerialDataSource(SerialDataSource *source)
{
    if (source == NULL) return;
    readIntoBuffer(source);

    if (!hasAFrameWorthOfData(source)) return;

    seekFrameStart(source);

    while (hasValidFrameAtTail(source))
    {
        SerialFrame frame = extractFrame(source);
        source->frameCallback(frame);
        closeSerialFrame(frame);
    }

    if (hasAFrameWorthOfData(source))
    {
        discardBytes(source, 1);
        printf("Warning: Serial data loss due to misaligned frame\n");
    }
}

SerialFrame awaitSerialDataSourceFrame(SerialDataSource *source)
{
    source->tip = source->tail = 0;
    source->isFull = false;

    while(!hasValidFrameAtTail(source))
    {
        if (!isSerialDataSourceConnected(source)) exit(EXIT_SUCCESS);
        if (hasAFrameWorthOfData(source)) discardBytes(source, 1);
        readIntoBuffer(source);

        seekFrameStart(source);
    }
    return extractFrame(source);
}