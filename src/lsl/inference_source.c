# include "lsl/inference_stream.h"
# include "lsl/helpers.h"
# include "tbci_build_config.h"

static LSLDataSource dataSource;

void initializeLslInferenceSource(void)
{
    dataSource = createLSLDataSource(INFERENCE_STREAM_PREDICATE);
}

bool pollLslInferenceSource(TinyBCIInference *out, uint64_t *microsecondTimestamp)
{
    if (pollLSLDataSource(&dataSource))
    {
        double *sample = (double*)dataSource.sampleBuffer;

        out->predictedLabel = (uint16_t)sample[0];
        out->confidence = (float)sample[1];
        for (int i = 0; i < N_OUTPUTS; i++)
        {
            out->confidences[i] = (float)sample[2 + i];
        }
        out->targetLabel = (uint16_t)sample[2 + N_CLASSES];
        *microsecondTimestamp = (uint64_t)sample[3 + N_CLASSES];

        return true;
    }
    return false;
}

void closeLslInferenceSource(void) { closeLSLDataSource(&dataSource); }

bool tryConnectLslInferenceSource(void) { return tryConnectLSLDataSource(&dataSource); }
bool isLslInferenceSourceConnected(void) { return isLSLDataSourceConnected(&dataSource); }
bool isLslInferenceSourceConsumable(void) { return isLSLDataSourceConsumable(&dataSource); }