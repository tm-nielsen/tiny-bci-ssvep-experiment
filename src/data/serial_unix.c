# include "data/serial.h"

# if defined(_WIN32) || defined(_WIN64)
# else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <time.h>

int serialOpen(SerialHandle *handle, const char *port, uint32_t readTimeout)
{
    char *fullPortPath = malloc(strlen(port) + 5);
    sprintf(fullPortPath, "/dev/%s", port);
    if (access(fullPortPath, F_OK) != 0) {
        fprintf(stderr, "Error: Port %s does not exist\n", port);
        return EXIT_FAILURE;
    }

    *handle = open(fullPortPath, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    free(fullPortPath);
    if (*handle == INVALID_HANDLE_VALUE) {
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
    if (*handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error: Attempted to write to invalid serial handle\n");
        return 0;
    }

    return write(*handle, buffer, bufferLength);
}

int serialRead(SerialHandle *handle, uint8_t *buffer, size_t bufferLength)
{
    if (*handle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error: Attempted to read from invalid serial handle\n");
        return 0;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(*handle, &readfds);

    struct timeval timeout = { .tv_sec = 0, .tv_usec = 50000 }; /* 50 ms */

    int ready = select(*handle + 1, &readfds, NULL, NULL, &timeout);
    if (ready <= 0)
        return 0;  /* timeout or error — no data */

    ssize_t n = read(*handle, buffer, bufferLength);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        if (errno == EBADF)
        {
            fprintf(stderr, "--\nSerial handle disconnected\n---\n");
            serialClose(handle);
            return 0;
        }
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