# include "data/synthetic_eeg_source.h"
# include "microsecond_timer.h"

static MicrosecondTimer timer = { .interval = SAMPLE_INTERVAL};
static float samples[CHANNEL_COUNT];
static uint32_t sampleIndex = 0;

static float tau = (float)(2 * TBCI_M_PI);

void resetSyntheticEEGSource()
{
    resetMicrosecondTimer(&timer);
}

void updateSyntheticEEGSource()
{
    if (checkMicrosecondTimer(&timer))
    {
        uint64_t now = getCurrentMicrosecondTimestamp();

        float currentSeconds = (float)now / 1000000.0f;
        for (uint16_t channelIndex = 0; channelIndex < CHANNEL_COUNT; channelIndex++)
        {
            float phaseOffset = channelIndex * (tau / CHANNEL_COUNT);
            float sineInput = tau * SIGNAL_FREQUENCY * currentSeconds + phaseOffset;
            samples[channelIndex] = SIGNAL_AMPLITUDE * (float)sin(sineInput);

            if (NOISE_AMPLITUDE > 0)
            {
                float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
                samples[channelIndex] += NOISE_AMPLITUDE * noise;
            }
        }

        in_push_signal(&tbciInputs, samples, now, sampleIndex++);
    }
}