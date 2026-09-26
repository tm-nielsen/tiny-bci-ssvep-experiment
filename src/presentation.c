# include "presentation.h"
# include "stimuli/stimulus_pattern.h"

static RenderTexture2D renderTarget;
static Texture2D renderTexture;
static Rectangle renderTextureRect;

static float *frequencies;
static uint16_t frequencyCount;

static uint16_t columnCount;
static Rectangle presenterSpacing;

static Texture2D stimulusTexture;
static Rectangle stimulusTextureSourceRect;
static Color stimulusTextureBackgroundColour;

static uint16_t targetIndex;
static bool hasTarget = false;

static uint16_t selectionIndex;
static double selectionTime = -SELECTION_DISPLAY_TIME;

static double stimulusStartTime = 0.0;
static float refreshRate = 60.0f;

static bool stimulusEnabled = true;
static bool textureEnabled = true;

// ---
# ifndef MIN
#   define MIN(a, b) ((a) < (b) ? (a) : (b))
# endif

static float getGridSize(float safeArea, uint16_t itemCount)
{
    return (safeArea - (GRID_GAP * (itemCount - 1))) / itemCount;
}

static void initializeWindow(void)
{
    SetTraceLogLevel(LOG_WARNING);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(RENDER_WIDTH, RENDER_HEIGHT, "Tiny BCI SSVEP Experiment");
    int monitorHz = GetMonitorRefreshRate(GetCurrentMonitor());
    if (monitorHz > 0) refreshRate = (float)monitorHz;
    SetWindowMinSize(MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT);

    renderTarget = LoadRenderTexture(RENDER_WIDTH, RENDER_HEIGHT);

    renderTexture = renderTarget.texture;
    renderTextureRect = (Rectangle){ 0, 0, RENDER_WIDTH, -RENDER_HEIGHT };
    SetTextureFilter(renderTexture, TEXTURE_FILTER_BILINEAR);
}

static void initializePresenters(const float *pFrequencies, uint16_t pFrequencyCount)
{
    frequencyCount = pFrequencyCount;
    size_t memorySize = frequencyCount * sizeof(float);
    frequencies = malloc(memorySize);
    memcpy(frequencies, pFrequencies, memorySize);

    columnCount = frequencyCount / ROW_COUNT;
    float width = getGridSize(SAFE_AREA_X, columnCount);
    float height = getGridSize(SAFE_AREA_Y, ROW_COUNT);

    presenterSpacing = (Rectangle)
    {
        width + GRID_GAP,
        height + GRID_GAP,
        width,
        height
    };
}

void initializeGaborPatches()
{
    CellPatternParams params = CELL_PATTERN_DEFAULTS;
    params.width  = (int)presenterSpacing.width;
    params.height = (int)presenterSpacing.height;

    stimulusTexture = GenerateCellPatternTexture(&params);
    stimulusTextureSourceRect = (Rectangle){ 0, 0, (float)stimulusTexture.width, (float)stimulusTexture.height };
    stimulusTextureBackgroundColour = params.backgroundColour;
}


void initializePresentation(const float *pFrequencies, uint16_t pFrequencyCount)
{
    initializeWindow();
    initializePresenters(pFrequencies, pFrequencyCount);
    initializeGaborPatches();
}

// ---

static Vector2 getGridOrigin(uint16_t index)
{
    uint16_t rowIndex = index % columnCount;
    uint16_t columnIndex = index / columnCount;

    return (Vector2)
    {
        MARGIN_SIDE + presenterSpacing.x * rowIndex,
        MARGIN_TOP + presenterSpacing.y * columnIndex
    };
}

static Vector2 getGridCentre(uint16_t index)
{
    Vector2 gridOrigin = getGridOrigin(index);
    return (Vector2)
    {
        gridOrigin.x + presenterSpacing.width / 2,
        gridOrigin.y + presenterSpacing.height / 2
    };
}

static Rectangle getGridRect(uint16_t index, int16_t padding)
{
    Vector2 gridOrigin = getGridOrigin(index);

    return (Rectangle)
    {
        gridOrigin.x - padding,
        gridOrigin.y - padding,
        presenterSpacing.width + 2 * padding,
        presenterSpacing.height + 2 * padding
    };
}

// ---

void setPresentationTarget(uint16_t index)
{
    targetIndex = index;
    hasTarget = true;
}
void clearPresentationTarget(void) { hasTarget = false; }

static void drawTargetIndicator(void)
{
    if (!hasTarget) return;

    Vector2 gridOrigin = getGridCentre(targetIndex);
    bool drawInverted = gridOrigin.y > RENDER_HEIGHT / 2;
    float yOffsetDirection = drawInverted ? -1.0f : 1.0f;

    Vector2 arrowTip = gridOrigin;
    arrowTip.y += presenterSpacing.height / 2 * yOffsetDirection;
    arrowTip.y += TARGET_INDICATION_OFFSET * yOffsetDirection;

    Vector2 arrowBottomLeft = (Vector2)
    {
        arrowTip.x - TARGET_INDICATION_SIZE.x / 2,
        arrowTip.y + TARGET_INDICATION_SIZE.y * yOffsetDirection
    };
    Vector2 arrowBottomRight = (Vector2)
    {
        arrowBottomLeft.x + TARGET_INDICATION_SIZE.x,
        arrowBottomLeft.y
    };

    if (drawInverted)
    DrawTriangle(arrowTip, arrowBottomRight, arrowBottomLeft, TARGET_INDICATION_COLOUR);
    else
    DrawTriangle(arrowTip, arrowBottomLeft, arrowBottomRight, TARGET_INDICATION_COLOUR);
}

// ---

void displaySelection(uint16_t index)
{
    selectionIndex = index;
    selectionTime = GetTime();
}

static void drawSelectionIndicator(void)
{
    if (GetTime() > selectionTime + SELECTION_DISPLAY_TIME) return;

    Rectangle borderRect = getGridRect(selectionIndex, SELECTION_DISPLAY_WIDTH);
    DrawRectangleRec(borderRect, SELECTION_DISPLAY_COLOUR);
}

static void drawStimulusBreakPlaceholder(uint16_t index)
{
    DrawRectangleRec(getGridRect(index, 0), STIMULUS_BREAK_PLACEHOLDER_COLOUR);
}

// ---

static void drawLetterboxedTarget(void)
{
    BeginDrawing();
        ClearBackground(LETTERBOX_COLOUR);

        float scaleX = (float)GetScreenWidth() / RENDER_WIDTH;
        float scaleY = (float)GetScreenHeight() / RENDER_HEIGHT;
        float scale = MIN(scaleX, scaleY);

        Rectangle letterboxRect = {
            (GetScreenWidth() - scale * RENDER_WIDTH) / 2,
            (GetScreenHeight() - scale * RENDER_HEIGHT) / 2,
            RENDER_WIDTH * scale,
            RENDER_HEIGHT * scale
        };
        
        DrawTexturePro(
            renderTexture, renderTextureRect, letterboxRect,
            (Vector2) {0, 0}, 0, WHITE
        );
    EndDrawing();
}

// ---

static void drawMessage(const char *message)
{
    int textWidth = MeasureText(message, MESSAGE_SCREEN_FONT_SIZE);
    DrawText(
        message,
        (RENDER_WIDTH - textWidth) / 2, RENDER_HEIGHT / 2,
        MESSAGE_SCREEN_FONT_SIZE, MESSAGE_SCREEN_TEXT_COLOUR
    );
}

void drawPreparationScreen(const char *message)
{
    BeginTextureMode(renderTarget);
        ClearBackground(BACKGROUND_COLOUR);
        
        for (uint16_t i = 0; i < frequencyCount; i++)
        {
            drawStimulusBreakPlaceholder(i);
        }
        drawTargetIndicator();
        drawMessage(message);
    EndTextureMode();

    drawLetterboxedTarget();
}

void drawMessageScreen(const char *message)
{
    BeginTextureMode(renderTarget);
        ClearBackground(MESSAGE_SCREEN_BACKGROUND_COLOUR);
        drawMessage(message);
    EndTextureMode();

    drawLetterboxedTarget();
}

// ---

static void drawStimulusPresenter(uint16_t index, double frameTime)
{
    Rectangle gridRect = getGridRect(index, 0);

    double waveValue = sin(frequencies[index] * TAU * (double)frameTime);
    float normalizedValue = (float)(waveValue + 1) / 2.0f;

    if (textureEnabled)
    {
        DrawRectangleRec(gridRect, BLACK);
        BeginBlendMode(BLEND_ADDITIVE);
        
        Color blendedBackgroundColour = stimulusTextureBackgroundColour;
        blendedBackgroundColour = ColorAlpha(blendedBackgroundColour, 1 - normalizedValue);
        DrawRectangleRec(gridRect, blendedBackgroundColour);

        Color textureColor = WHITE;
        textureColor = ColorAlpha(textureColor, normalizedValue);
        DrawTexturePro(stimulusTexture, stimulusTextureSourceRect, gridRect, (Vector2){0, 0}, 0, textureColor);
        EndBlendMode();
    }
    else
    {
        Color stimulusColor = ColorLerp(
            UNTEXTURED_STIMULUS_OFF_COLOUR,
            UNTEXTURED_STIMULUS_ON_COLOUR,
            normalizedValue
        );
        DrawRectangleRec(gridRect, stimulusColor);
    }
}

void drawStimulusScreen(void)
{
    double t = GetTime() - stimulusStartTime;
    double frameTime = llround(t * refreshRate) / (double)refreshRate;

    BeginTextureMode(renderTarget);
        ClearBackground(BACKGROUND_COLOUR);

        drawSelectionIndicator();

        for (uint16_t i = 0; i < frequencyCount; i++)
        {
            if (stimulusEnabled) drawStimulusPresenter(i, frameTime);
            else drawStimulusBreakPlaceholder(i);
        }

        drawTargetIndicator();
    EndTextureMode();

    drawLetterboxedTarget();
}

void pauseStimulus(void) { stimulusEnabled = false; }
void resumeStimulus(void) { stimulusEnabled = true; stimulusStartTime = GetTime(); }

void disableTextureStimulus(void) { textureEnabled = false; }
void enableTextureStimulus(void) { textureEnabled = true; }

// ---

void stopPresentation(void)
{
    free(frequencies);
    UnloadRenderTexture(renderTarget);
    UnloadTexture(stimulusTexture);
    CloseWindow();
}