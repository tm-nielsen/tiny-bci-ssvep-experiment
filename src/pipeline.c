# include "pipeline.h"
# include "data/eeg_source.h"
# include "data/trigger_source.h"

void initializeTinyBCIPipelineStorage();
void setTinyBCIPipelineConfiguration();
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

int initializeTinyBCIPipeline(const float *frequencies)
{
    initializeTinyBCIPipelineStorage();
    setTinyBCIPipelineConfiguration();
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
    sb_init(&signalBuffer, signalStorage, signalTimestamps, signalIndices, SIG_CAPACITY, CHANNEL_COUNT);
    sb_init(&processedSignalBuffer, processedSignalStorage, processedSignalTimestamps, processedSignalIndices, SIG_CAPACITY, CHANNEL_COUNT);
    tq_init(&triggerQueue, triggerStorage, TRIG_CAPACITY);
    eq_init(&epochQueue, epochStorage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&epochQueue, epochPool, CHANNEL_COUNT);
    eq_init(&featuresQueue, featuresStorage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&featuresQueue, featuresPool, CHANNEL_COUNT);
    eq_init(&outputQueue, outputStorage, EPOCH_CAPACITY, TOTAL_FRAMES);
    eq_configure(&outputQueue, outputPool, CHANNEL_COUNT);

    tbciInputs.signal = &signalBuffer;
    tbciInputs.triggers = &triggerQueue;
    tbciInputs.n_channels = CHANNEL_COUNT;
}

void setTinyBCIPipelineConfiguration()
{
    tbciConfiguration.paradigm = TBCI_PARADIGM_SSVEP;
    tbciConfiguration.nominal_srate = SAMPLE_RATE;
    tbciConfiguration.target_srate = SAMPLE_RATE;
    tbciConfiguration.n_channels = CHANNEL_COUNT;
    tbciConfiguration.window_length_ms = PRE_MS + POST_MS;
    tbciConfiguration.mode = SEG_MODE_SLIDING;
    tbciConfiguration.pre_stimulus_ms = PRE_MS;
    tbciConfiguration.post_stimulus_ms = POST_MS;
    tbciConfiguration.overlap_ms = 400u;
    tbciConfiguration.trial_end_code = TRIAL_END_CODE;
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