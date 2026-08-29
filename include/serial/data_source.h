# pragma once
# ifndef SERIAL_DATA_SOURCE
# define SERIAL_DATA_SOURCE

# include "serial.h"
# define MAKE_OXFF_TERMINATED_BYTE_ARRAY(...) (uint8_t[]){ __VA_ARGS__, 0xFF }

typedef struct {
    SerialHandle handle;
    void (*frameCallback)(SerialFrame);

    uint8_t *startBytes;
    uint8_t *endBytes;
    uint16_t frameSize;

    uint8_t *buffer;
    uint32_t bufferLength;
    uint16_t tip;
    uint16_t tail;
    bool isFull;
} SerialDataSource;

typedef struct {
    uint32_t index;
    uint8_t *buffer;
    uint16_t size;
} SerialFrame;

SerialDataSource createSerialDataSource(
    uint16_t frameSize,
    uint8_t *startBytes, uint8_t *endBytes
);
SerialDataSource createSerialDataSourceFromOpenHandle(
    SerialHandle handle, uint16_t frameSize,
    uint8_t *startBytes, uint8_t *endBytes
);
int openSerialDataSource(
    SerialDataSource *source,
    const char* port, uint32_t readTimeout
);
void updateSerialDataSource(SerialDataSource *source);
void closeSerialDataSource(SerialDataSource *source);
void closeSerialDataSourceButNotHandle(SerialDataSource *source);

SerialFrame awaitSerialDataSourceFrame(SerialDataSource *source);

bool isSerialDataSourceConnected(SerialDataSource *source);
void setSerialDataSourceCallback(SerialDataSource *source, void (*callback)(SerialFrame));

SerialFrame createSerialFrame(uint16_t size);
void closeSerialFrame(SerialFrame frame);

# endif