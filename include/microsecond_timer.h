# pragma once
uint64_t getCurrentMicrosecondTimestamp(void);

typedef struct {
    uint64_t interval;
    uint64_t nextTimeout;
} MicrosecondTimer;

bool checkMicrosecondTimer(MicrosecondTimer *timer);
void resetMicrosecondTimer(MicrosecondTimer *timer);

MicrosecondTimer createMicrosecondTimer(float);