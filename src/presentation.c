# include "presentation.h"

static float *frequencies;
static uint16_t frequencyCount;

static uint16_t columnCount;
static Rectangle presenterSpacing;

static uint16_t selectionIndex;
static double selectionTime;

// ---

float getGridSize(float safeArea, uint16_t itemCount)
{
    return (safeArea - (GRID_GAP * (itemCount - 1))) / itemCount;
}

void initializePresentation(const float *pFrequencies, uint16_t pFrequencyCount)
{
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Tiny BCI SSVEP Experiment");

    frequencyCount = pFrequencyCount;
    size_t memorySize = frequencyCount * sizeof(float);
    frequencies = malloc(memorySize);
    memcpy(frequencies, frequencies, memorySize);

    columnCount = frequencyCount / ROW_COUNT;
    float width = getGridSize(SAFE_AREA_X, columnCount);
    float height = getGridSize(SAFE_AREA_Y, ROW_COUNT);

    presenterSpacing = (Rectangle){
        width + GRID_GAP,
        height + GRID_GAP,
        width,
        height
    };
}

// ---

Rectangle getGridRect(uint16_t index, uint16_t padding)
{
    uint16_t rowIndex = index % columnCount;
    uint16_t columnIndex = index / columnCount;

    return (Rectangle)
    {
        MARGIN_SIDE + presenterSpacing.x * rowIndex - padding,
        MARGIN_TOP + presenterSpacing.y * columnIndex - padding,
        presenterSpacing.width + 2 * padding,
        presenterSpacing.height + 2 * padding
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