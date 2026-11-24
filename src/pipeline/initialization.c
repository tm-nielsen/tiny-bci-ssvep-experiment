# include "pipeline.h"
# include "program/constants.h"
# include "tbci_build_config.h"

_Static_assert(WINDOW_OVERLAP_MS < WINDOW_LENGTH_MS, "overlap must be shorter than the window");

static bool initializeTinyBCIPipelineStorage(uint8_t, uint32_t);
static void setTinyBCIPipelineConfiguration(uint8_t, float, int, int, int);
# if TBCI_PARADIGM_ID == TBCI_PARADIGM_SSVEP_ID
static bool addCCANodesToTinyBCIPipeline(const float *frequencies);
_Static_assert(sizeof((float[])FREQUENCIES) / sizeof(float) == N_CLASSES, "one frequency per class");
# elif TBCI_PARADIGM_ID == TBCI_PARADIGM_P300_ID
static bool addERPNodesToTinyBCIPipeline(uint8_t channelCount);
# elif TBCI_PARADIGM_ID == TBCI_PARADIGM_MI_ID
static bool addMotorImageryNodesToTinyBCIPipeline(uint8_t channelCount);
# else
#   error "Unknown TBCI_PARADIGM_ID"
# endif

int initializeTinyBCIPipeline(const float *classes, uint8_t channelCount, uint32_t sampleRate)
{
    if (!initializeTinyBCIPipelineStorage(channelCount, sampleRate)) return -1;

# if TBCI_PARADIGM_ID == TBCI_PARADIGM_SSVEP_ID
    setTinyBCIPipelineConfiguration(channelCount, (float)sampleRate, TBCI_PARADIGM_SSVEP, SEG_MODE_SLIDING, WINDOW_OVERLAP_MS);
    if (!addCCANodesToTinyBCIPipeline(classes)) return -1;
# elif TBCI_PARADIGM_ID == TBCI_PARADIGM_P300_ID
    (void)classes;
    setTinyBCIPipelineConfiguration(channelCount, (float)sampleRate, TBCI_PARADIGM_P300, SEG_MODE_TRIGGERED, 0);
    if (!addERPNodesToTinyBCIPipeline(channelCount)) return -1;
# elif TBCI_PARADIGM_ID == TBCI_PARADIGM_MI_ID
    (void)classes;
    setTinyBCIPipelineConfiguration(channelCount, (float)sampleRate, TBCI_PARADIGM_MI, SEG_MODE_SLIDING, WINDOW_OVERLAP_MS);
    if (!addMotorImageryNodesToTinyBCIPipeline(channelCount)) return -1;
# endif

    TBCI_Status status = tbci_context_init(
        &tbciContext, &tbciConfiguration,
        &tbciInputs, &processedSignalBuffer,
        &epochQueue, &featuresQueue, &outputQueue
    );
    return reportAndReturnPipelineStatus(status, "initialize");
}

static bool initializeTinyBCIPipelineStorage(uint8_t channelCount, uint32_t sampleRate)
{
    if (!allocateDynamicStorage(channelCount, sampleRate)) return false;

    sb_init(&signalBuffer, signalStorage, signalTimestamps, signalIndices, SIG_CAPACITY, channelCount);
    sb_init(&processedSignalBuffer, processedSignalStorage, processedSignalTimestamps, processedSignalIndices, SIG_CAPACITY, channelCount);
    tq_init(&triggerQueue, triggerStorage, TRIG_CAPACITY);
    eq_init(&epochQueue, epochStorage, EPOCH_CAPACITY, totalFrames);
    eq_configure(&epochQueue, epochPool, channelCount);
    eq_init(&featuresQueue, featuresStorage, EPOCH_CAPACITY, totalFrames);
    eq_configure(&featuresQueue, featuresPool, channelCount);
    eq_init(&outputQueue, outputStorage, EPOCH_CAPACITY, totalFrames);
    eq_configure(&outputQueue, outputPool, channelCount);

    tbciInputs.signal = &signalBuffer;
    tbciInputs.triggers = &triggerQueue;
    tbciInputs.n_channels = channelCount;

    return true;
}

static void setTinyBCIPipelineConfiguration(uint8_t channelCount, float sampleRate, int paradigm, int mode, int overlapMs)
{
    tbciConfiguration.paradigm = paradigm;
    tbciConfiguration.nominal_srate = sampleRate;
    tbciConfiguration.target_srate = sampleRate;
    tbciConfiguration.n_channels = channelCount;
    tbciConfiguration.window_length_ms = WINDOW_LENGTH_MS;
    tbciConfiguration.mode = mode;
    tbciConfiguration.pre_stimulus_ms = 0;
    tbciConfiguration.post_stimulus_ms = WINDOW_LENGTH_MS;
    tbciConfiguration.overlap_ms = overlapMs;
    tbciConfiguration.trial_end_code = TRIAL_END_CODE;
    tbciConfiguration.use_preprocessing = true;
    tbciConfiguration.use_feature_extraction = true;
    tbciConfiguration.use_decoder = true;
    tbciConfiguration.log_enabled = false; /* set true to enable CSV logging */
    tbciConfiguration.log_processed = false; /* set true to enable logging of preprocessed data */
}

# if TBCI_PARADIGM_ID == TBCI_PARADIGM_SSVEP_ID
static bool addCCANodesToTinyBCIPipeline(const float *frequencies)
{
    notchConfiguration.freq_hz = NOTCH_FREQ;
    notchConfiguration.q_factor = 10.0f;
    notchConfiguration.n_harmonics = 1;
    notch_init(&notchNode, &notchConfiguration);

    bp_configure(&bandpassConfiguration, 2.0f, 45.0f, 3);
    bp_init(&bandpassNode, &bandpassConfiguration);

    ccaConfiguration.n_freqs = N_CLASSES;
    ccaConfiguration.n_harmonics = N_HARMONICS;
    for (uint16_t i = 0; i < N_CLASSES; i++) ccaConfiguration.freqs[i] = frequencies[i];
    cca_init(&ccaNode, &ccaConfiguration, refSignals, referenceSignalsCapacity);

    ccaModelConfiguration.n_freqs     = N_CLASSES;
    ccaModelConfiguration.min_excess  = 0.15f;
    ccaModelConfiguration.excess_temp = 0.05f;
    TBCI_Status s = cca_model_init(&ccaModel, &ccaModelConfiguration);
    if (s != TBCI_OK) { fprintf(stderr, "cca_model_init failed: %d\n", s); return false; }

    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&notchNode);
    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&bandpassNode);
    group_add_node(&tbciContext.features.group,      (TBCI_Node *)&ccaNode);
    group_add_node(&tbciContext.decoder.group,       (TBCI_Node *)&ccaModel);
    return true;
}
# endif

# if TBCI_PARADIGM_ID == TBCI_PARADIGM_P300_ID
static bool addERPNodesToTinyBCIPipeline(uint8_t channelCount)
{
    notchConfiguration.freq_hz = NOTCH_FREQ;
    notchConfiguration.q_factor = 10.0f;
    notchConfiguration.n_harmonics = 1;
    notch_init(&notchNode, &notchConfiguration);

    bp_configure(&bandpassConfiguration, 1.0f, 15.0f, 3);
    bp_init(&bandpassNode, &bandpassConfiguration);

    labelEncoderConfiguration.binarize_target = true;
    TBCI_Status s = le_init(&labelEncoderNode, &labelEncoderConfiguration);
    if (s != TBCI_OK) { fprintf(stderr, "LabelEncoder init failed: %d\n", s); return false; }

    snprintf(onnxModelConfiguration.model_path, TBCI_ONNX_MAX_PATH_LEN, "%s", TBCI_MODEL_PATH);
    onnxModelConfiguration.output_mode       = TBCI_OUTPUT_SIGMOID;
    onnxModelConfiguration.sigmoid_threshold = 0.5f;
    onnxModelConfiguration.temperature       = 1.0f;
    onnxModelConfiguration.train_trials      = NULL;   /* inference only */
    onnxModelConfiguration.train_labels      = NULL;
    onnxModelConfiguration.train_capacity    = 0;
    onnxModelConfiguration.n_folds           = 0;
    onnxModelConfiguration.scorer            = NULL;
    s = onnx_model_init(&onnxModel, &onnxModelConfiguration, &tbciContext);
    if (s != TBCI_OK) { fprintf(stderr, "OnnxModel init failed: %d\n", s); return false; }

    if (onnxModel.input_size != (size_t)channelCount * totalFrames)
    {
        fprintf(stderr, "model input %zu does not match %u channels x %zu frames\n",
                onnxModel.input_size, channelCount, totalFrames);
        return false;
    }

    trialCycleConfiguration.n_stimuli  = N_CLASSES;
    trialCycleConfiguration.max_reps   = 5;
    trialCycleConfiguration.p_stop     = 0.95f;
    trialCycleConfiguration.idle_prior = 0.5f;
    trialCycleConfiguration.llr_scale  = 1.0f;
    s = tc_init(&trialCycleNode, &trialCycleConfiguration);
    if (s != TBCI_OK) { fprintf(stderr, "TrialCycle init failed: %d\n", s); return false; }

    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&notchNode);
    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&bandpassNode);
    group_add_node(&tbciContext.decoder.group,       (TBCI_Node *)&labelEncoderNode);
    group_add_node(&tbciContext.decoder.group,       (TBCI_Node *)&onnxModel);
    group_add_node(&tbciContext.decoder.group,       (TBCI_Node *)&trialCycleNode);
    return true;
}
# endif

# if TBCI_PARADIGM_ID == TBCI_PARADIGM_MI_ID
static bool addMotorImageryNodesToTinyBCIPipeline(uint8_t channelCount)
{
    notchConfiguration.freq_hz = NOTCH_FREQ;
    notchConfiguration.q_factor = 10.0f;
    notchConfiguration.n_harmonics = 1;
    notch_init(&notchNode, &notchConfiguration);

    bp_configure(&bandpassConfiguration, 8.0f, 30.0f, 3);   // mu + beta: must match training
    bp_init(&bandpassNode, &bandpassConfiguration);

    // Only needed if the trigger codes aren't already the class labels 1..N
    labelEncoderConfiguration.binarize_target = false;        // keep every cue class
    TBCI_Status s = le_init(&labelEncoderNode, &labelEncoderConfiguration);
    if (s != TBCI_OK) { fprintf(stderr, "LabelEncoder init failed: %d\n", s); return false; }

    snprintf(onnxModelConfiguration.model_path, TBCI_ONNX_MAX_PATH_LEN, "%s", TBCI_MODEL_PATH);
    onnxModelConfiguration.output_mode    = TBCI_OUTPUT_SOFTMAX;   // GUESS: your multi-class output mode
    onnxModelConfiguration.temperature    = 1.0f;
    onnxModelConfiguration.train_trials   = NULL;                  // inference only
    onnxModelConfiguration.train_labels   = NULL;
    onnxModelConfiguration.train_capacity = 0;
    onnxModelConfiguration.n_folds        = 0;
    onnxModelConfiguration.scorer         = NULL;
    s = onnx_model_init(&onnxModel, &onnxModelConfiguration, &tbciContext);
    if (s != TBCI_OK) { fprintf(stderr, "OnnxModel init failed: %d\n", s); return false; }

    if (onnxModel.input_size != (size_t)channelCount * totalFrames)
    {
        fprintf(stderr, "model input %zu does not match %u channels x %zu frames\n",
                onnxModel.input_size, channelCount, totalFrames);
        return false;
    }

    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&notchNode);
    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&bandpassNode);
    group_add_node(&tbciContext.decoder.group,       (TBCI_Node *)&labelEncoderNode);
    group_add_node(&tbciContext.decoder.group,       (TBCI_Node *)&onnxModel);
    return true;
}
# endif