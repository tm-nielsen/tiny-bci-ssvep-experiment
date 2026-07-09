# include "presentation.h"

void initializePresentation(int width, int height)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(width, height, "Tiny BCI SSVEP Experiment");
}

void updatePresentation()
{
    bool on = sin(8 * 2.0 * PI * GetTime()) > 0;

    BeginDrawing();

    ClearBackground(BACKGROUND_COLOUR);
    DrawRectangle(50, 50, 100, 80, on ? STIMULUS_ON_COLOR : STIMULUS_OFF_COLOUR);

    EndDrawing();
}