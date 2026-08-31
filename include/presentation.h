# pragma once
# include "raylib.h"

# define RENDER_WIDTH 600
# define RENDER_HEIGHT 400
# define MINIMUM_WINDOW_WIDTH 600
# define MINIMUM_WINDOW_HEIGHT 400

# define MESSAGE_SCREEN_FONT_SIZE 24
# define MESSAGE_SCREEN_TEXT_COLOUR (Color){255, 255, 255, 255}
# define MESSAGE_SCREEN_BACKGROUND_COLOUR (Color){48, 48, 64, 255}

# define ROW_COUNT 2
# define MARGIN_TOP 50
# define MARGIN_BOTTOM 50
# define MARGIN_SIDE 50
# define GRID_GAP 100

# define SAFE_AREA_X (RENDER_WIDTH - 2 * MARGIN_SIDE)
# define SAFE_AREA_Y (RENDER_HEIGHT - (MARGIN_TOP + MARGIN_BOTTOM))

# define BACKGROUND_COLOUR (Color){32, 32, 32, 255}
# define LETTERBOX_COLOUR (Color){0, 0, 0, 255}

# define UNTEXTURED_STIMULUS_ON_COLOUR (Color){255, 255, 255, 255}
# define UNTEXTURED_STIMULUS_OFF_COLOUR (Color){0, 0, 0, 255}
# define TEXTURE_STIMULUS_FILEPATH "assets/gabor-stimulus.png"
# define STIMULUS_BREAK_PLACEHOLDER_COLOUR (Color){64, 64, 80, 255}

# define TARGET_INDICATION_OFFSET 10
# define TARGET_INDICATION_SIZE (Vector2){30, 20}
# define TARGET_INDICATION_COLOUR (Color){255, 80, 80, 255}

# define SELECTION_DISPLAY_WIDTH 12
# define SELECTION_DISPLAY_COLOUR (Color){120, 200, 120, 255}
# define SELECTION_DISPLAY_TIME 0.5f

# define TAU 6.28318530717958647692528676655900576839433879875021

void initializePresentation(const float *pFrequencies, uint16_t pFrequencyCount);
void drawPreparationScreen(const char *message);
void drawMessageScreen(const char *message);
void drawStimulusScreen(void);
void stopPresentation(void);

void pauseStimulus(void);
void resumeStimulus(void);

void disableTextureStimulus(void);
void enableTextureStimulus(void);

void setPresentationTarget(uint16_t index);
void clearPresentationTarget(void);
void displaySelection(uint16_t index);