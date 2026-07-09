# include "microsecond_timer.h"
# include "pipeline.h"
# include "producer.h"
# include "presentation.h"


int main(int argc, char *argv[])
{
    const uint32_t targetEpochs = 10;
    const bool sliding = false;
    const float frequencies[N_FREQS] = {7.0f, 8.0f, 9.0f, 11.0f, 7.5f, 8.5f};

    if (initializeTinyBCIPipeline(sliding, frequencies)) return EXIT_FAILURE;
    if (initializeTinyBCIProducer(sliding)) return EXIT_FAILURE;

    if (startTinyBCIPipeline()) return EXIT_FAILURE;

    printf("Tiny BCI Pipeline Running.\n");

    size_t epochsCollected = 0;
    uint64_t nextTick = getCurrentMicrosecondTimestamp();
    uint64_t tickSpacing = (uint64_t)(1000000.0f / SRATE);

    initializePresentation(800, 450);

    while (!WindowShouldClose() && (epochsCollected < targetEpochs || targetEpochs == 0))
    {
        uint64_t now = getCurrentMicrosecondTimestamp();
        if (now >= nextTick)
        {
            if (updateTinyBCIProducer()) break;
            if (updateTinyBCIPipeline()) break;

            
            uint16_t inferenceLabel;
            if (tryGetTinyBCIInference(&inferenceLabel))
            {
                printf("Output received: %d\n", inferenceLabel);
                epochsCollected++;
            }

            nextTick += tickSpacing;
        }

        updatePresentation(now);
    }

    closeTinyBCIProducer();
    stopTinyBCIPipeline();

    printf("\nDone. Collected %zu / %u epochs.\n", epochsCollected, targetEpochs);

    CloseWindow();
    return EXIT_SUCCESS;
}