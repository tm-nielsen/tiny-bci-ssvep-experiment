# include "serial/serial.h"

# if defined(_WIN32) || defined(_WIN64)
# else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <time.h>

int serialOpen(SerialHandle *handle, const char *port, uint32_t readTimeout)
{
    *handle = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (isSerialHandleInvalid(handle)) {
        fprintf(stderr, "Error: Cannot open %s — %s (errno=%d)\n",
                port, strerror(errno), errno);
        return EXIT_FAILURE;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    tcgetattr(*handle, &tty);

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag  = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(TERMIOS_C_ANTIFLAGS);
    tty.c_iflag  = IGNBRK;
    tty.c_lflag  = 0;
    tty.c_oflag  = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    tcsetattr(*handle, TCSANOW, &tty);
    return EXIT_SUCCESS;
}

// ---

int serialWrite(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    if (isSerialHandleInvalid(handle))
    {
        fprintf(stderr, "Error: Attempted to write to invalid serial handle\n");
        return 0;
    }

    return write(*handle, buffer, bufferLength);
}

int serialRead(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    if (isSerialHandleInvalid(handle))
    {
        fprintf(stderr, "Error: Attempted to read from invalid serial handle\n");
        return 0;
    }

    ssize_t n = read(*handle, buffer, bufferLength);

    if (n <= 0)
    {
        struct stat sb;
        fstat(*handle, &sb);
        if (sb.st_nlink == 0)
        {
            fprintf(stderr, "--\nSerial handle disconnected\n---\n");
            serialClose(handle);
            return 0;
        }
    }

    if (n < 0) {
        printf("read error: %u\n", errno);
        return 0;
    }
    return (int)n;
}

void serialFlush(SerialHandle *handle) { tcflush(*handle, TCIFLUSH); }
void serialClose(SerialHandle *handle)
{
    close(*handle);
    *handle = INVALID_HANDLE_VALUE;
}

// --

void sleepMilliseconds(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000u;
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    nanosleep(&ts, NULL);
}

# endif