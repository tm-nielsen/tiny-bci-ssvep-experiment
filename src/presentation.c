# include "presentation.h"

void initializePresentation(int width, int height)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(width, height, "Tiny BCI SSVEP Experiment");
}

void updatePresentation(uint64_t microsecondTimestamp)
{
    BeginDrawing();

    ClearBackground(BACKGROUND_COLOUR);
    DrawRectangle(50, 50, 100, 80, STIMULUS_ON_COLOR);

    EndDrawing();
}