#ifndef STIMULUS_PATTERN_H
#define STIMULUS_PATTERN_H

#include "raylib.h"
#include <stdint.h>

typedef struct
{
    int width, height;        // texture size in pixels (match the on-screen stimulus size for crisp edges)
    float cellsAcross;        // number of cells across the texture HEIGHT (density)
    float blobRadius;         // blob radius as a fraction of a cell (0.02 .. 0.5)
    float elongation;         // 0 = round grains, 1..2 = rods
    float sizeVariation;      // 0 = all equal, 1 = very mixed
    float jitter;             // 0 = regular grid, 1 = fully scattered
    float softnessPx;         // edge softness in pixels (>= 1 for anti-aliasing)
    float envelopeSigma;      // Gaussian envelope sigma as a fraction of height; 0 = no envelope
    uint32_t seed;            // different seed = different layout
    Color blobColour;
    Color backgroundColour;
} CellPatternParams;

// Sensible defaults: rod bacteria, white on black, no envelope
#define CELL_PATTERN_DEFAULTS (CellPatternParams){ \
.width = 512, .height = 512, .cellsAcross = 8.0f, .blobRadius = 0.12f, .elongation = 1.6f, \
.sizeVariation = 0.2f, .jitter = 0.9f, .softnessPx = 1.0f, .envelopeSigma = 0.3f, .seed = 42u, \
.blobColour = WHITE, .backgroundColour = DARKGRAY }

Image GenerateCellPatternImage(const CellPatternParams *params);

Texture2D GenerateCellPatternTexture(const CellPatternParams *params);

#endif