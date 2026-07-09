# include "presentation.h"

static float *frequencies;
static uint16_t frequencyCount;

static uint16_t columnCount;
static uint16_t presenterWidth;
static uint16_t presenterHeight;
static uint16_t presenterSpacingX;
static uint16_t presenterSpacingY;


void initializePresentation(const float *pFrequencies, uint16_t pFrequencyCount)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Tiny BCI SSVEP Experiment");

    frequencyCount = pFrequencyCount;
    size_t memorySize = frequencyCount * sizeof(float);
    frequencies = malloc(memorySize);
    memcpy(frequencies, frequencies, memorySize);

    columnCount = frequencyCount / ROW_COUNT;
    presenterWidth = (SAFE_AREA_X - (GRID_GAP * (columnCount - 1))) / columnCount;
    presenterHeight = (SAFE_AREA_Y - (GRID_GAP * (ROW_COUNT - 1))) / ROW_COUNT;
    presenterSpacingX = presenterWidth + GRID_GAP;
    presenterSpacingY = presenterHeight + GRID_GAP;
}

// ---

void drawStimulusPresenter(uint16_t index)
{
    bool on = sin(frequencies[index] * TAU * GetTime()) > 0;
    Color colour = on ? STIMULUS_ON_COLOR : STIMULUS_OFF_COLOUR;

    uint16_t rowIndex = index % columnCount;
    uint16_t columnIndex = index / columnCount;
    uint16_t positionX = MARGIN_SIDE + presenterSpacingX * rowIndex;
    uint16_t positionY = MARGIN_TOP + presenterSpacingY * columnIndex;
    DrawRectangle(positionX, positionY, presenterWidth, presenterHeight, colour);
}

void updatePresentation()
{
    BeginDrawing();

    ClearBackground(BACKGROUND_COLOUR);
    for (uint16_t i = 0; i < frequencyCount; i++)
    {
        drawStimulusPresenter(i);
    }

    EndDrawing();
}

// ---

void stopPresentation()
{
    free(frequencies);
    CloseWindow();
}