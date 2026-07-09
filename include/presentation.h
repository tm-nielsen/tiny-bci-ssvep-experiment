# pragma once
# include "raylib.h"

# define BACKGROUND_COLOUR (Color){20, 20, 20, 255}
# define STIMULUS_ON_COLOR (Color){255, 255, 255, 255}
# define STIMULUS_OFF_COLOUR (Color){0, 0, 0, 255}

void initializePresentation(int, int);
void updatePresentation(uint64_t);