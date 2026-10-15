# include "inference_logger.h"
# include <time.h>

static FILE *logFile = NULL;
static uint16_t currentTarget = 0;
static uint64_t targetAssignmentTimestamp = 0;
static uint16_t roundNumber = 0;

InferenceRecord makeInferenceRecord(TinyBCIInference inference, uint64_t timestamp)
{
    InferenceRecord record = {
        .timestamp_us = timestamp,
        .delta_time_us = timestamp - targetAssignmentTimestamp,
        .true_label = (uint16_t)(currentTarget + 1),
        .predicted_label = (uint16_t)(inference.predictedLabel + 1),
        .round_number = roundNumber,
        .confidence = inference.confidence,
    };
    for (int i = 0; i < N_FREQS; i++) record.probabilities[i] = inference.confidences[i];
    return record;
}

# ifndef TBCI_PICO

void initializeInferenceLogger(void)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timeString[32];
    strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S", t);

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "tbci_inference_%s_%s_%s.csv",
        tbciContext.core_config.log_subject,
        tbciContext.core_config.log_session,
        timeString
    );

    logFile = fopen(filepath, "w");

    if (logFile) {
        fprintf(logFile, "timestamp_us,delta_time_us,true_label,predicted_label,confidence,round_number");
        for (int i = 0; i < N_FREQS; i++)
        {
            fprintf(logFile, ",prob_%d", i);
        }
        printf("Inference Logger: logging to '%s'\n", filepath);
    }
    else
    {
        fprintf(stderr, "Failed to open inference logger file.\n");
        exit(EXIT_FAILURE);
    }
}

void closeInferenceLogger(void)
{
    if (logFile)
    {
        fclose(logFile);
        logFile = NULL;
        printf("Inference Logger: file saved.\n");
    }
}

# endif

void notifyInferenceLoggerOfNewTarget(uint16_t target, uint64_t timestamp)
{
    if (target == 0 && currentTarget != 0) roundNumber++;
    currentTarget = target;
    targetAssignmentTimestamp = timestamp;
}

void logInference(TinyBCIInference inference, uint64_t timestamp)
{
    if (!logFile)
    {
        printf("Failed to log inference due to failed initialization.\n");
        return;
    }

    InferenceRecord r = makeInferenceRecord(inference, timestamp);
    fprintf(logFile, "\n%" PRIu64 ",%" PRIu64 ",%u,%u,%.6f,%u",
        r.timestamp_us, r.delta_time_us, r.true_label, r.predicted_label,
        r.confidence, r.round_number);
    for (int i = 0; i < N_FREQS; i++) fprintf(logFile, ",%.6f", r.probabilities[i]);
    fflush(logFile);
}

