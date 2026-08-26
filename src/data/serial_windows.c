#include "data/serial.h"

#if defined(_WIN32) || defined(_WIN64)

int serialOpen(SerialHandle *handle, const char *port, uint32_t readTimeout)
{
    *handle = CreateFileA(port,
        GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
    );
    if (*handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error: Failed to open serial port %s\n", port);
        return EXIT_FAILURE;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    GetCommState(*handle, &dcb);
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    SetCommState(*handle, &dcb);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = readTimeout;
    SetCommTimeouts(*handle, &timeouts);

    return EXIT_SUCCESS;
}

// ---

int serialWrite(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    if (*handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error: Attempted to write to invalid serial handle\n");
        return 0;
    }

    DWORD writeCount = 0;
    WriteFile(*handle, buffer, (DWORD)bufferLength, &writeCount, NULL);
    return writeCount;
}

int serialRead(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    if (*handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error: Attempted to read from invalid serial handle\n");
        return 0;
    }

    DWORD readCount = 0;
    ReadFile(*handle, buffer, (DWORD)bufferLength, &readCount, NULL);

    DWORD lastError = GetLastError();
    if (lastError != ERROR_SUCCESS)
    {
        if (
            lastError == ERROR_OPERATION_ABORTED ||
            lastError == ERROR_ACCESS_DENIED ||
            lastError == ERROR_INVALID_HANDLE
        ) {
            fprintf(stderr, "--\nSerial handle disconnected\n---\n");
            serialClose(handle);
            return 0;
        }
    }
    return readCount;
}

void serialFlush(SerialHandle *handle) { PurgeComm(*handle, PURGE_RXCLEAR); }
void serialClose(SerialHandle *handle)
{
    CloseHandle(*handle);
    *handle = INVALID_HANDLE_VALUE;
}

// ---

void sleepMilliseconds(uint32_t ms) { Sleep(ms); }

#endif