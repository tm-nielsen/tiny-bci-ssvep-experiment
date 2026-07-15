# include "data/unicorn_eeg_source.h"

#include "microsecond_timer.h"
#include "data/synthetic_eeg_source.h"

static UnicornProducer producer;
static UnicornProducerConfig config;
static MicrosecondTimer timer = { .interval = SAMPLE_INTERVAL };


void initializeUnicornEEGSource(const char *port)
{
    config.port       = port;
    config.srate      = UNICORN_SRATE;
    config.n_channels = UNICORN_N_CHANNELS;

    up_init(&producer, &config);
    producer.trigger_gen = NULL;  // trigger_source.c / trial_conductor.c drive triggers instead

    TBCI_Status s = producer.base.init((TBCI_Producer *)&producer, &tbciInputs, &tbciContext);
    if (s != TBCI_OK)
    {
        fprintf(stderr, "unicorn: failed to connect on %s | code: %d\n", port, s);
    }
}

void resetUnicornEEGSource()
{
    up_reset(&producer);
    producer.state.timestamp_us = getCurrentMicrosecondTimestamp();
}

void updateUnicornEEGSource()
{
    if (checkMicrosecondTimer(&timer))
    {
        uint64_t now = getCurrentMicrosecondTimestamp();
        TBCI_Status s = producer.base.tick((TBCI_Producer *)&producer, &tbciInputs, &tbciContext);
        if (s != TBCI_OK && s != TBCI_ERR_EMPTY)
        {
            fprintf(stderr, "unicorn: tick error | code: %d\n", s);
        }
    }
}

void closeUnicornEEGSource()
{
    up_close(&producer);
}