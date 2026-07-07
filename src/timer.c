# include "timer.h"

# if defined(_WIN32)
uint64_t getCurrentMicrosecondTimestamp()
{
    FILETIME fileTime;
    uint64_t time;

    GetSystemTimePreciseAsFileTime(&fileTime);
    time = fileTime.dwLowDateTime;
    time += ((uint64_t)fileTime.dwHighDateTime) << 32;
    
    return time / 10;
}

# else
uint64_t getCurrentMicrosecondTimestamp()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

# endif