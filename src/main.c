# include "pipeline.h"
# include "producer.h"
# include "presentation.h"


int main(int argc, char *argv[])
{
    const bool sliding = false;
    const float frequencies[N_FREQS] = {7.0f, 8.0f, 9.0f, 11.0f, 7.5f, 8.5f};

    if (initializeTinyBCIPipeline(sliding, frequencies)) return EXIT_FAILURE;
    if (initializeTinyBCIProducer(sliding)) return EXIT_FAILURE;

    if (startTinyBCIPipeline()) return EXIT_FAILURE;
    printf("Tiny BCI Pipeline Running.\n");

    initializePresentation(800, 450);
    while (!WindowShouldClose())
    {
        if (updateTinyBCIProducer()) break;
        if (updateTinyBCIPipeline()) break;

        
        uint16_t inferenceLabel;
        if (tryGetTinyBCIInference(&inferenceLabel))
        {
            printf("Output received: %d\n", inferenceLabel);
        }

        updatePresentation(now);
    }

    closeTinyBCIProducer();
    stopTinyBCIPipeline();
    CloseWindow();

    return EXIT_SUCCESS;
}