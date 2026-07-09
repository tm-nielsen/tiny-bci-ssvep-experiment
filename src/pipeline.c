# include "pipeline.h"

void initializeTinyBCIPipelineStorage();
void setTinyBCIPipelineConfiguration(bool);
void addCCANodesToTinyBCIPipeline(const float *);

int reportStatus(TBCI_Status status, const char *actionLabel)
{
    if (status)
    {
        fprintf(stderr, "Failed to %s Tiny BCI Pipeline | code: %d\n", actionLabel, status);
    }
    return status;
}

// ---

int initializeTinyBCIPipeline(bool sliding, const float *frequencies)
{
    initializeTinyBCIPipelineStorage();
    setTinyBCIPipelineConfiguration(sliding);
    addCCANodesToTinyBCIPipeline(frequencies);

    TBCI_Status initializationStatus = tbci_context_init(
        &tbciContext, &tbciConfiguration,
        &tbciInputs, &processedSignalBuffer,
        &epochQueue, &featuresQueue, &outputQueue
    );
    tbciInputs.signal = &signalBuffer;
    return reportStatus(initializationStatus, "initialize");
}

void initializeTinyBCIPipelineStorage()
{
    sb_init(&signalBuffer, signalStorage, signalTimestamps, signalIndices, SIG_CAPACITY, N_CHANNELS);
    sb_init(&processedSignalBuffer, processedSignalStorage, processedSignalTimestamps, processedSignalIndices, SIG_CAPACITY, N_CHANNELS);
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
}

void setTinyBCIPipelineConfiguration(bool sliding)
{
    tbciConfiguration.paradigm = sliding ? TBCI_PARADIGM_MI : TBCI_PARADIGM_P300;
    tbciConfiguration.nominal_srate = SRATE;
    tbciConfiguration.target_srate = SRATE;
    tbciConfiguration.n_channels = N_CHANNELS;
    tbciConfiguration.window_length_ms = PRE_MS + POST_MS;
    tbciConfiguration.mode = sliding ? SEG_MODE_SLIDING : SEG_MODE_TRIGGERED;
    tbciConfiguration.pre_stimulus_ms = PRE_MS;
    tbciConfiguration.post_stimulus_ms = POST_MS;
    tbciConfiguration.overlap_ms = sliding ? 400u : 0u;
    tbciConfiguration.trial_end_code = sliding ? 10u : 0u;
    tbciConfiguration.use_preprocessing = true;
    tbciConfiguration.use_feature_extraction = true;
    tbciConfiguration.use_decoder = true;
    tbciConfiguration.log_enabled = false; /* set true to enable CSV logging */
    tbciConfiguration.log_subject[0] = '\0';
    tbciConfiguration.log_session[0] = '\0';
}

void addCCANodesToTinyBCIPipeline(const float *frequencies)
{
    /* register notch & bandpass node in preprocessing group */
    notchConfiguration.freq_hz = 50.0f;
    notch_init(&notchNode, &notchConfiguration);

    bandpassConfiguration.low_hz = 1.0f;
    bandpassConfiguration.high_hz = 40.0f;
    bp_init(&bandpassNode, &bandpassConfiguration);

    /* Register CCA node and model */
    ccaConfiguration.n_freqs = N_FREQS;
    ccaConfiguration.n_harmonics = N_HARMONICS;
    for (uint16_t i = 0; i < N_FREQS; i++)
    {
        ccaConfiguration.freqs[i] = frequencies[i];
    }

    cca_init(&ccaNode, &ccaConfiguration, ref_signals, REF_CAP);

    ccaModelConfiguration.temperature = 0.3f;
    cca_model_init(&ccaModel, &ccaModelConfiguration);

    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&notchNode);
    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&bandpassNode);
    group_add_node(&tbciContext.decoder.group, (TBCI_Node *)&ccaNode);
    group_add_node(&tbciContext.decoder.group, (TBCI_Node *)&ccaModel);
}

// ---

int startTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_start(&tbciContext, TBCI_STATE_INFERENCE);
    return reportStatus(status, "start");
}
int startTinyBCIPipelineInState(TBCI_State initialState)
{
    TBCI_Status status = tbci_context_start(&tbciContext, initialState);
    return reportStatus(status, "start");
}

int updateTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_tick(&tbciContext);
    return reportStatus(status, "update");
}

int stopTinyBCIPipeline()
{
    TBCI_Status status = tbci_context_stop(&tbciContext);
    return reportStatus(status, "stop");
}

// ---

bool tryGetTinyBCIInference(uint16_t *out)
{
    if (eq_is_empty(&outputQueue))
        return false;

    TBCI_Epoch epoch;
    eq_pop(&outputQueue, &epoch);
    *out = epoch.label;
    return true;
}