# include "serial/serial.h"

bool isSerialHandleInvalid(SerialHandle *handle)
{
    return *handle == INVALID_HANDLE_VALUE;
}