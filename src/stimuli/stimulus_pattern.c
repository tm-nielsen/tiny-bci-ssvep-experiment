#include "stimuli/stimulus_pattern.h"
#include <math.h>
#include <stdlib.h>


// Integer hash -> uniform float in [0, 1). Deterministic across platforms.
static uint32_t hashCell(int32_t x, int32_t y, uint32_t salt)
{
    uint32_t h = (uint32_t)x * 0x8da6b343u ^ (uint32_t)y * 0xd8163841u ^ salt * 0xcb1ab31fu;
    h ^= h >> 16; h *= 0x7feb352du;
    h ^= h >> 15; h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

static float randomCell(int32_t x, int32_t y, uint32_t seed, uint32_t channel)
{
    return (float)(hashCell(x, y, seed * 8u + channel) >> 8) * (1.0f / 16777216.0f);
}

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static float smoothstepf(float e0, float e1, float x)
{
    float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Signed distance (in cell units) from point (u, v) to the nearest blob. Negative = inside.
static float cellsDistance(float u, float v, const CellPatternParams *p)
{
    int32_t ix = (int32_t)floorf(u);
    int32_t iy = (int32_t)floorf(v);
    float fx = u - (float)ix;
    float fy = v - (float)iy;

    float best = 1e9f;
    for (int oy = -1; oy <= 1; oy++)
    for (int ox = -1; ox <= 1; ox++)
    {
        int32_t cx = ix + ox, cy = iy + oy;

        float centreX = (float)ox + 0.5f + (randomCell(cx, cy, p->seed, 0) - 0.5f) * p->jitter;
        float centreY = (float)oy + 0.5f + (randomCell(cx, cy, p->seed, 1) - 0.5f) * p->jitter;
        float angle   = randomCell(cx, cy, p->seed, 2) * 2.0f * PI;
        float radius  = p->blobRadius * lerpf(1.0f - p->sizeVariation, 1.0f, randomCell(cx, cy, p->seed, 3));
        float halfLen = p->elongation * radius * lerpf(0.5f, 1.5f, randomCell(cx, cy, p->seed, 4));

        float dirX = cosf(angle), dirY = sinf(angle);
        float dx = fx - centreX, dy = fy - centreY;

        // Capsule: distance to a segment of half-length halfLen, minus the radius
        float t = clampf(dx * dirX + dy * dirY, -halfLen, halfLen);
        float ex = dx - dirX * t, ey = dy - dirY * t;
        float d = sqrtf(ex * ex + ey * ey) - radius;
        if (d < best) best = d;
    }
    return best;
}

Image GenerateCellPatternImage(const CellPatternParams *p)
{
    Image image = GenImageColor(p->width, p->height, p->backgroundColour);
    Color *pixels = (Color *)image.data;   // GenImageColor -> PIXELFORMAT_UNCOMPRESSED_R8G8B8A8

    float pixelsPerCell = (float)p->height / p->cellsAcross;
    float soft = fmaxf(p->softnessPx, 0.5f);
    float cx = p->width * 0.5f, cy = p->height * 0.5f;
    float sigmaPx = p->envelopeSigma * (float)p->height;

    for (int y = 0; y < p->height; y++)
    for (int x = 0; x < p->width; x++)
    {
        // Pixel centre in cell units (square cells, based on height)
        float u = ((float)x + 0.5f) / pixelsPerCell;
        float v = ((float)y + 0.5f) / pixelsPerCell;

        float distPx = cellsDistance(u, v, p) * pixelsPerCell;
        float cover = 1.0f - smoothstepf(-soft, soft, distPx);   // anti-aliased edge

        if (sigmaPx > 0.0f)
        {
            float rx = (float)x + 0.5f - cx, ry = (float)y + 0.5f - cy;
            cover *= expf(-(rx * rx + ry * ry) / (2.0f * sigmaPx * sigmaPx));
        }

        Color bg = p->backgroundColour, fg = p->blobColour;
        pixels[y * p->width + x] = (Color)
        {
            (unsigned char)lerpf(bg.r, fg.r, cover),
            (unsigned char)lerpf(bg.g, fg.g, cover),
            (unsigned char)lerpf(bg.b, fg.b, cover),
            (unsigned char)lerpf(bg.a, fg.a, cover)
        };
    }
    return image;
}

Texture2D GenerateCellPatternTexture(const CellPatternParams *p)
{
    Image image = GenerateCellPatternImage(p);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}