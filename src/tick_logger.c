/**
 * @file tick_logger.c
 *
 * @brief Pipeline tick timing for Linux builds (Pi 4, Pi Zero 2 W). See tick_logger.h.
 */

#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE   // macOS hides CLOCK_MONOTONIC_RAW when _POSIX_C_SOURCE is set

#include "tick_logger.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define TICK_BUFFER_SIZE 8192   // about 30 s of ticks at 250 Hz between writes

// RAW is not adjusted by NTP; fall back to the plain monotonic clock where it is missing.
#ifdef CLOCK_MONOTONIC_RAW
#define TICK_CLOCK CLOCK_MONOTONIC_RAW
#else
#define TICK_CLOCK CLOCK_MONOTONIC
#endif

typedef struct
{
    uint64_t endNs;
    uint32_t durationUs;
    uint8_t isDecision;
} TickRecord;

static TickRecord tickBuffer[TICK_BUFFER_SIZE];
static size_t tickCount = 0;
static FILE *tickFile = NULL;
static uint64_t originNs = 0;      // time of the first logged tick; host_time_s counts from here
static double warmupSeconds = 0.0;

uint64_t tickLoggerNowNs(void)
{
    struct timespec ts;
    clock_gettime(TICK_CLOCK, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void writeTickBuffer(void)
{
    for (size_t i = 0; i < tickCount; i++)
    {
        const TickRecord *tick = &tickBuffer[i];
        fprintf(tickFile, "%.6f,%c,%u\n",
                (double)(tick->endNs - originNs) / 1e9,
                tick->isDecision ? 'D' : 'R',
                tick->durationUs);
    }
    fflush(tickFile);
    tickCount = 0;
}

void closeTickLogger(void)
{
    if (tickFile == NULL) return;
    writeTickBuffer();
    fclose(tickFile);
    tickFile = NULL;
}

// Ctrl+C / kill: exit() runs atexit handlers, which close the log.
static void onTerminationSignal(int signalNumber)
{
    (void)signalNumber;
    exit(EXIT_SUCCESS);
}

// Only takes over signals nobody else handles, so an existing handler is left alone.
static void installSignalIfDefault(int signalNumber)
{
    struct sigaction current;
    if (sigaction(signalNumber, NULL, &current) == 0 && current.sa_handler == SIG_DFL)
        signal(signalNumber, onTerminationSignal);
}

void initTickLogger(const char *label, double warmup)
{
    char host[64] = "host";
    gethostname(host, sizeof host - 1);

    char stamp[32];
    time_t now = time(NULL);
    strftime(stamp, sizeof stamp, "%Y%m%d_%H%M%S", localtime(&now));

    char path[256];
    snprintf(path, sizeof path, "tbci_ticks_%s_%s_%s.csv", label, host, stamp);

    tickFile = fopen(path, "w");
    if (tickFile == NULL)
    {
        perror("tick logger: cannot open file");
        return;
    }
    fprintf(tickFile, "host_time_s,kind,duration_us\n");

    warmupSeconds = warmup;
    originNs = 0;
    tickCount = 0;

    atexit(closeTickLogger);
    installSignalIfDefault(SIGINT);
    installSignalIfDefault(SIGTERM);

    printf("tick logger: writing %s (warm-up %.1f s)\n", path, warmup);
}

void logTick(uint64_t startNs, uint64_t endNs, bool isDecision)
{
    if (tickFile == NULL) return;

    if (originNs == 0) originNs = startNs;   // warm-up starts at the first pipeline tick
    if ((double)(startNs - originNs) / 1e9 < warmupSeconds) return;

    TickRecord *tick = &tickBuffer[tickCount++];
    tick->endNs = endNs;
    tick->durationUs = (uint32_t)((endNs - startNs) / 1000);
    tick->isDecision = isDecision ? 1 : 0;

    if (tickCount == TICK_BUFFER_SIZE) writeTickBuffer();
}