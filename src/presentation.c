# include "presentation.h"

static float *frequencies;
static uint16_t frequencyCount;

static uint16_t columnCount;
static uint16_t presenterWidth;
static uint16_t presenterHeight;
static uint16_t presenterSpacingX;
static uint16_t presenterSpacingY;

static uint16_t selectionIndex;
static double selectionTime;


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

Rectangle getGridRect(uint16_t index, uint16_t padding)
{
    uint16_t rowIndex = index % columnCount;
    uint16_t columnIndex = index / columnCount;

    return (Rectangle)
    {
        (float)(MARGIN_SIDE + presenterSpacingX * rowIndex - padding),
        (float)(MARGIN_TOP + presenterSpacingY * columnIndex - padding),
        (float)(presenterWidth + 2 * padding),
        (float)(presenterHeight + 2 * padding)
    };
}

// ---

void drawSelection()
{
    if (GetTime() > selectionTime + SELECTION_DISPLAY_TIME) return;

    Rectangle borderRect = getGridRect(selectionIndex, SELECTION_DISPLAY_WIDTH);
    DrawRectangleRec(borderRect, SELECTION_DISPLAY_COLOUR);
}

void displaySelection(uint16_t index)
{
    selectionIndex = index;
    selectionTime = GetTime();
}

// ---

void drawStimulusPresenter(uint16_t index)
{
    double waveValue = sin(frequencies[index] * TAU * GetTime());
    double weightedValue = (sqrt(fabs(waveValue)) * (waveValue / fabs(waveValue)));
    float normalizedValue = (float)(weightedValue + 1) / 2.0f;
    Color colour = ColorLerp(STIMULUS_ON_COLOR, STIMULUS_OFF_COLOUR, normalizedValue);

    DrawRectangleRec(getGridRect(index, 0), colour);
}

void updatePresentation()
{
    BeginDrawing();

    ClearBackground(BACKGROUND_COLOUR);
    drawSelection();

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