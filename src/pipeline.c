# include "pipeline.h"
#include <sys/time.h>
# include "data/eeg_source.h"
# include "data/trigger_source.h"

FILE *inferenceLog = NULL;


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
    // TODO code integration
    /* open inference log */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "tbci_inference_%s_%s_%s.csv",
             tbciConfiguration.log_subject,
             tbciConfiguration.log_session,
             timestamp);
    inferenceLog = fopen(log_path, "w");
    if (inferenceLog) {
        fprintf(inferenceLog, "timestamp_us,true_label,predicted_label,confidence");
        for (int i = 0; i < N_FREQS; i++)
            fprintf(inferenceLog, ",prob_%d", i);
        fprintf(inferenceLog, "\n");
        printf("Inference log: %s\n", log_path);
    }

    initializeTinyBCIPipelineStorage();
    setTinyBCIPipelineConfiguration();
    addCCANodesToTinyBCIPipeline(frequencies);

    TBCI_Status initializationStatus = tbci_context_init(
        &tbciContext, &tbciConfiguration,
        &tbciInputs, &processedSignalBuffer,
        &epochQueue, &featuresQueue, &outputQueue
    );
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
    tbciConfiguration.trial_end_code = TRIAL_END_CODE;
    tbciConfiguration.nominal_srate = SAMPLE_RATE;
    tbciConfiguration.target_srate = SAMPLE_RATE;
    tbciConfiguration.n_channels = CHANNEL_COUNT;
    tbciConfiguration.window_length_ms = WINDOW_LENGTH_MS;
    tbciConfiguration.mode = SEG_MODE_SLIDING;
    tbciConfiguration.pre_stimulus_ms = 0;
    tbciConfiguration.post_stimulus_ms = WINDOW_LENGTH_MS;
    tbciConfiguration.overlap_ms = WINDOW_OVERLAP_MS;
    tbciConfiguration.trial_end_code = TRIAL_END_CODE;
    tbciConfiguration.use_preprocessing = true;
    tbciConfiguration.use_feature_extraction = true;
    tbciConfiguration.use_decoder = true;
    tbciConfiguration.log_enabled = true; /* set true to enable CSV logging */
    tbciConfiguration.log_processed = false; /* set true to enable logging of preprocessed data for debugging */
    tbciConfiguration.log_subject[0] = '\0';
    tbciConfiguration.log_session[0] = '\0';
}

void addCCANodesToTinyBCIPipeline(const float *frequencies)
{
    /* register notch & bandpass node in preprocessing group */
    notchConfiguration.freq_hz = 60.0f;
    notchConfiguration.q_factor = 10.f;
    notchConfiguration.n_harmonics = 2;
    notch_init(&notchNode, &notchConfiguration);

    bp_configure(&bandpassConfiguration,2.0f, 45.0f, 3);
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
    ccaModelConfiguration.n_freqs = N_FREQS;
    cca_model_init(&ccaModel, &ccaModelConfiguration);

    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&notchNode);
    group_add_node(&tbciContext.preprocessing.group, (TBCI_Node *)&bandpassNode);
    group_add_node(&tbciContext.features.group, (TBCI_Node *)&ccaNode);
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
    if (inferenceLog) {
        fclose(inferenceLog);
        inferenceLog = NULL;
    }
    TBCI_Status status = tbci_context_stop(&tbciContext);
    return reportStatus(status, "stop");
}

// ---

bool tryGetTinyBCIInference(TinyBCIInference *out)
{
    if (eq_is_empty(&outputQueue))
        return false;

    TBCI_Epoch epoch;
    eq_pop(&outputQueue, &epoch);

    *out = (TinyBCIInference)
    {
        .predictedLabel = epoch.predicted_label,
        .targetLabel = epoch.label,
        .confidence = epoch.confidence
    };
    for (int i = 0; i < N_FREQS; i++)
    {
        out->confidences[i] = epoch.samples[i];
    }

    return true;
}