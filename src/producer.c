# include "producer.h"
# include "tbci_producer_factory.h"

TBCI_Producer *producer = (TBCI_Producer *)&syntheticProducer;

int initializeTinyBCIProducer(bool sliding)
{
    producerConfiguration.n_channels = N_CHANNELS;
    producerConfiguration.srate = SRATE;
    producerConfiguration.freq_hz = 10.0f;
    producerConfiguration.amplitude = 1.0f;
    producerConfiguration.noise_amplitude = 0.3f; /* 10% of signal amplitude */

    TBCI_Status producerInitializationStatus = tbci_producer_create_synthetic(
        producer, &producerConfiguration,
        &tbciInputs, &tbciContext
    );
    if (producerInitializationStatus)
    {
        fprintf(stderr, "Failed to initialize producer | code: %d\n", producerInitializationStatus);
        return producerInitializationStatus;
    }

    triggerGeneratorConfiguration.trigger_interval_ms = sliding ? 2000u : 1000u;
    triggerGeneratorConfiguration.trigger_code = 1u;
    triggerGeneratorConfiguration.trial_end_code = sliding ? 10u : 0u;
    triggerGeneratorConfiguration.trial_duration_ms = sliding ? 3000u : 0u;

    TBCI_Status triggerGeneratorInitializationStatus = tg_init(
        &triggerGenerator, &triggerGeneratorConfiguration, &triggerGeneratorState
    );
    if (triggerGeneratorInitializationStatus)
    {
        fprintf(stderr, "Failed to initialize trigger generator | code: %d\n", triggerGeneratorInitializationStatus);
        return triggerGeneratorInitializationStatus;
    }
    syntheticProducer.trigger_gen = &triggerGenerator;

    return TBCI_OK;
}

int updateTinyBCIProducer()
{
    TBCI_Status producerStatus = producer->tick(producer, &tbciInputs, &tbciContext);
    if (producerStatus)
    {
        fprintf(stderr, "Producer Error | code: %d\n", producerStatus);
    }
    return producerStatus;
}

int closeTinyBCIProducer()
{
    TBCI_Status producerStatus = producer->close(producer);
    if (producerStatus)
    {
        fprintf(stderr, "Failed to close producer | code: %d\n", producerStatus);
    }
    return producerStatus;
}