# pragma once
# include "raylib.h"

# define WINDOW_WIDTH 1200
# define WINDOW_HEIGHT 800

# define ROW_COUNT 2
# define MARGIN_TOP 100
# define MARGIN_BOTTOM 150
# define MARGIN_SIDE 100
# define GRID_GAP 200

# define SAFE_AREA_X (WINDOW_WIDTH - 2 * MARGIN_SIDE)
# define SAFE_AREA_Y (WINDOW_HEIGHT - (MARGIN_TOP + MARGIN_BOTTOM))

# define BACKGROUND_COLOUR (Color){20, 20, 20, 255}
# define STIMULUS_ON_COLOR (Color){255, 255, 255, 255}
# define STIMULUS_OFF_COLOUR (Color){0, 0, 0, 255}

# define TARGET_INDICATION_OFFSET 20
# define TARGET_INDICATION_SIZE (Vector2){80, 50}
# define TARGET_INDICATION_COLOUR (Color){255, 80, 80, 255}

# define SELECTION_DISPLAY_WIDTH 20
# define SELECTION_DISPLAY_COLOUR (Color){120, 200, 120, 255}
# define SELECTION_DISPLAY_TIME 0.5f

# define TAU 6.28318530717958647692528676655900576839433879875021

void initializePresentation(const float *, uint16_t);
void updatePresentation();
void stopPresentation();

void setPresentationTarget(uint16_t);
void displaySelection(uint16_t);