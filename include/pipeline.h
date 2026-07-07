# pragma once
# include "storage.h"
// # include "../producer/tbci_producer_factory.h"

static int initializePipeline(bool sliding)
{
    /* buffers */
    sb_init(&signalBuffer, signalStorage, signalTimestamps, signalIndices,SIG_CAPACITY, N_CHANNELS);
    sb_init(&processedSignalBuffer, processedSignalStorage, processedSignalTimestamps, processedSignalIndices,SIG_CAPACITY, N_CHANNELS);
    tq_init(&triggerQueue, triggerStorage, TRIG_CAPACITY);
    eq_init(&epochQueue, epochStorage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&epochQueue, epochPool, N_CHANNELS);
    eq_init(&featuresQueue, featuresStorage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&featuresQueue, featuresPool, N_CHANNELS);
    eq_init(&outputQueue, outputStorage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&outputQueue, outputPool, N_CHANNELS);

    tbciInputs.signal = &signalBuffer;
    tbciInputs.triggers = &triggerQueue;
    tbciInputs.n_channels = N_CHANNELS;

    tbciConfiguration.paradigm = sliding ? TBCI_PARADIGM_MI : TBCI_PARADIGM_P300;
    tbciConfiguration.nominal_srate = SRATE;
    tbciConfiguration.target_srate = SRATE;
    tbciConfiguration.n_channels = N_CHANNELS;
    tbciConfiguration.window_length_ms = PRE_MS + POST_MS;
    tbciConfiguration.mode = sliding ? SEG_MODE_SLIDING : SEG_MODE_TRIGGERED;
    tbciConfiguration.pre_stimulus_ms = PRE_MS;
    tbciConfiguration.post_stimulus_ms = POST_MS;
    tbciConfiguration.overlap_ms = sliding ? 400u : 0u;
    tbciConfiguration.trial_end_code = sliding ? 10u  : 0u;
    tbciConfiguration.use_preprocessing = true;
    tbciConfiguration.use_feature_extraction = true;
    tbciConfiguration.use_decoder = true;
    tbciConfiguration.log_enabled = false;  /* set true to enable CSV logging */
    tbciConfiguration.log_subject[0] = '\0';
    tbciConfiguration.log_session[0] = '\0';

    /* register notch & bandpass node in preprocessing group */
    notchConfiguration.freq_hz = 50.0f;
    notch_init(&notchNode, &notchConfiguration);

    bandpassConfiguration.low_hz  = 1.0f;
    bandpassConfiguration.high_hz = 40.0f;
    bp_init(&bandpassNode, &bandpassConfiguration);

    /* Register CCA node and model */
    ccaConfiguration.n_freqs = N_FREQS;
    ccaConfiguration.n_harmonics = N_HARMONICS;
    ccaConfiguration.freqs[0] = 7.0f;
    ccaConfiguration.freqs[4] = 7.5f;
    ccaConfiguration.freqs[1] = 8.0f;
    ccaConfiguration.freqs[5] = 8.5f;
    ccaConfiguration.freqs[2] = 9.0f;
    ccaConfiguration.freqs[3] = 11.0f;

    cca_init(&ccaNode, &ccaConfiguration, ref_signals, REF_CAP);

    ccaModelConfiguration.temperature = 0.3f;
    cca_model_init(&ccaModel, &ccaModelConfiguration);

    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&notchNode);
    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&bandpassNode);
    group_add_node(&tbciContext.decoder.group, (TBCI_Node *)&ccaNode);
    group_add_node(&tbciContext.decoder.group, (TBCI_Node *)&ccaModel);

    TBCI_Status s = tbci_context_init(&tbciContext, &tbciConfiguration, &tbciInputs, &processedSignalBuffer, &epochQueue, &featuresQueue, &outputQueue);
    if (s != TBCI_OK) return s;

    // producerConfiguration.n_channels = N_CHANNELS;
    // producerConfiguration.srate = SRATE;
    // producerConfiguration.freq_hz = 10.0f;
    // producerConfiguration.amplitude = 1.0f;
    // producerConfiguration.noise_amplitude = 0.3f;  /* 10% of signal amplitude */

    // // tbci_producer_create_synthetic(producer, &producerConfiguration, &tbciInputs, &tbciContext);

    // triggerGeneratorConfiguration.trigger_interval_ms = sliding ? 2000u : 1000u;
    // triggerGeneratorConfiguration.trigger_code = 1u;
    // triggerGeneratorConfiguration.trial_end_code = sliding ? 10u   : 0u;
    // triggerGeneratorConfiguration.trial_duration_ms = sliding ? 3000u : 0u;

    // tg_init(&triggerGenerator, &triggerGeneratorConfiguration, &triggerGeneratorState);
    // syntheticProducer.trigger_gen = &triggerGenerator;

    return TBCI_OK;
}