# pragma once
# include "raylib.h"

# define WINDOW_WIDTH 800
# define WINDOW_HEIGHT 450

# define ROW_COUNT 2
# define MARGIN_TOP 20
# define MARGIN_BOTTOM 80
# define MARGIN_SIDE 20
# define GRID_GAP 50

# define SAFE_AREA_X (WINDOW_WIDTH - 2 * MARGIN_SIDE)
# define SAFE_AREA_Y (WINDOW_HEIGHT - (MARGIN_TOP + MARGIN_BOTTOM))

# define BACKGROUND_COLOUR (Color){20, 20, 20, 255}
# define STIMULUS_ON_COLOR (Color){255, 255, 255, 255}
# define STIMULUS_OFF_COLOUR (Color){0, 0, 0, 255}

# define TAU 6.28318530717958647692528676655900576839433879875021

void initializePresentation(const float *, uint16_t);
void updatePresentation();
void stopPresentation();