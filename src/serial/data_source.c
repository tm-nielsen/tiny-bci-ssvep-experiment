# include "serial/data_source.h"

uint8_t count0xFFTerminatedBytes(uint8_t *bytes)
{
    uint8_t count = 0;
    while (bytes[count++] != 0xFF);
    return count;
}

static uint8_t * copyBytes(uint8_t *bytes)
{
    int16_t count = 0;
    while (bytes[count++] != 0xFF);
    
    uint8_t *storage = malloc(count);
    while (--count >= 0) storage[count] = bytes[count];
    return storage;
}

SerialDataSource createSerialDataSourceFromOpenHandle(
    SerialHandle handle, uint16_t frameSize,
    uint8_t *startBytes, uint8_t *endBytes
)
{
    uint16_t bufferLength = frameSize * 2;

    return (SerialDataSource) {
        .handle = handle,
        .frameCallback = NULL,
        .startBytes = copyBytes(startBytes),
        .endBytes = copyBytes(endBytes),
        .frameSize = frameSize,
        .bufferLength = bufferLength,
        .buffer = malloc(bufferLength),
        .tip = 0,
        .tail = 0,
        .isFull = false
    };
}

SerialDataSource createSerialDataSource(
    uint16_t frameSize,
    uint8_t *startBytes, uint8_t *endBytes
) {
    return createSerialDataSourceFromOpenHandle(
        INVALID_HANDLE_VALUE, frameSize,
        startBytes, endBytes
    );
}

int openSerialDataSource(
    SerialDataSource *source,
    const char* port, uint32_t readTimeout
) {
    if (source == NULL) return EXIT_FAILURE;
    if (source->buffer == NULL)
    {
        fprintf(stderr, "Error: Invalid buffer state\n");
        return EXIT_FAILURE;
    }
    return serialOpen(&(source->handle), port, readTimeout);
}

void resetSerialDataSource(SerialDataSource *source)
{
    source->tip = source->tail = 0;
    source->isFull = false;
}

void closeSerialDataSource(SerialDataSource *source)
{
    if (source == NULL) return;
    serialClose(&(source->handle));

    closeSerialDataSourceButNotHandle(source);
}

void closeSerialDataSourceButNotHandle(SerialDataSource *source)
{
    if (source == NULL) return;
    free(source->buffer);
    free(source->startBytes);
    free(source->endBytes);

    source->buffer = NULL;
    source->bufferLength = 0;
    source->tip = 0;
    source->tail = 0;

    source->frameCallback = NULL;
    source->startBytes = NULL;
    source->endBytes = NULL;
    source->isFull = false;
}

bool isSerialDataSourceConnected(SerialDataSource *source)
{
    return source != NULL && source->handle != INVALID_HANDLE_VALUE;
}

void setSerialDataSourceCallback(SerialDataSource *source, void (*callback)(SerialFrame))
{
    if (source != NULL) source->frameCallback = callback;
}

// ---

SerialFrame createSerialFrame(uint16_t size)
{
    return (SerialFrame)
    {
        .buffer = malloc(size),
        .size = size
    };
}

void closeSerialFrame(SerialFrame frame)
{
    free(frame.buffer);
    frame.size = 0;
}