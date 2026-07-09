# include "raylib.h"

# include "tbci_common.h"

# include "pipeline.h"
# include "microsecond_timer.h"


int main(int argc, char *argv[])
{
    const uint32_t targetEpochs = 10;
    const bool sliding = false;

    if (initializePipeline(sliding) != TBCI_OK)
    {
        fprintf(stderr, "Failed to initialize Tiny BCI pipeline.\n");
        return EXIT_FAILURE;
    }
    if (tbci_context_start(&tbciContext, TBCI_STATE_INFERENCE) != TBCI_OK)
    {
        fprintf(stderr, "Failed to start Tiny BCI pipeline.\n");
        return EXIT_FAILURE;
    }

    printf("Tiny BCI Pipeline Running.\n");

    size_t epochsCollected = 0;
    uint64_t nextTick = getCurrentMicrosecondTimestamp();
    uint64_t tickSpacing = (uint64_t)(1000000.0f / SRATE);


    const int screenWidth = 800;
    const int screenHeight = 450;

    const Color backgroundColour = {20, 20, 20, 255};
    const Color stimulusColour = {255, 255, 255, 255};

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(screenWidth, screenHeight, "Tiny BCI SSVEP Experiment");

    while (!WindowShouldClose() && (epochsCollected < targetEpochs || targetEpochs == 0))
    {
        uint64_t now = getCurrentMicrosecondTimestamp();
        if (now >= nextTick)
        {
            TBCI_Status producerStatus = producer->tick(producer, &tbciInputs, &tbciContext);
            if (producerStatus != TBCI_OK)
            {
                fprintf(stderr, "Producer Error.\n");
                break;
            }

            TBCI_Status pipelineStatus = tbci_context_tick(&tbciContext);
            if (pipelineStatus != TBCI_OK)
            {
                fprintf(stderr, "Pipeline Error.\n");
                break;
            }

            while (!eq_is_empty(&outputQueue))
            {
                TBCI_Epoch epoch;
                eq_pop(&outputQueue, &epoch);
                printf("Output received: %d\n", epoch.label);
                epochsCollected++;
            }

            nextTick += tickSpacing;
        }

        BeginDrawing();

        ClearBackground(backgroundColour);
        DrawRectangle(50, 50, 100, 80, stimulusColour);

        EndDrawing();
    }

    producer->close(producer);
    tbci_context_stop(&tbciContext);

    printf("\nDone. Collected %zu / %u epochs.\n", epochsCollected, targetEpochs);

    CloseWindow();
    return EXIT_SUCCESS;
}