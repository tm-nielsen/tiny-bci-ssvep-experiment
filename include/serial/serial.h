# pragma once
# ifndef SERIAL
# define SERIAL

# if defined(_WIN32) || defined(_WIN64)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
    typedef HANDLE SerialHandle;
# else
#   include <termios.h>
    typedef int SerialHandle;
#   define INVALID_HANDLE_VALUE (-1)

#   ifdef CRTSCTS
#       define TERMIOS_C_ANTIFLAGS (PARENB | CSTOPB | CRTSCTS)
#   else
#       define TERMIOS_C_ANTIFLAGS (PARENB | CSTOPB)
#   endif
# endif

int serialOpen(SerialHandle *handle, const char *port, uint32_t readTimeout);

int serialWrite(SerialHandle *handle, uint8_t *buffer, size_t bufferLength);
int serialRead(SerialHandle *handle, uint8_t *buffer, size_t bufferLength);

void serialFlush(SerialHandle *handle);
void serialClose(SerialHandle *handle);

void sleepMilliseconds(uint32_t ms);

bool isSerialHandleInvalid(SerialHandle *handle);

# endif