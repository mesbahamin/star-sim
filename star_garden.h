#ifndef STAR_GARDEN_H

#include <inttypes.h>
#include <math.h>
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "star.h"
#include "barnes_hut.h"

#define FULLSCREEN
#define RESPAWN_STARS
#define USE_TEST_SEED

#define TITLE "Stars"
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define BYTES_PER_PIXEL 4
#define NUM_STARS 100
#define DRAG 1

#define SECOND 1000.0f
#define FPS 60
#define MS_PER_FRAME (SECOND / FPS)
#define UPDATES_PER_SECOND 120
#define MS_PER_UPDATE (SECOND / UPDATES_PER_SECOND)

#define COLOR_WHITE      0xFFFFFF
#define COLOR_BLACK      0x000000
#define COLOR_SOL_BG     0x002B36
#define COLOR_YELLOW     0xB58900
#define COLOR_ORANGE     0xCB4B16
#define COLOR_RED        0xDC322F
#define COLOR_MAGENTA    0xD33682
#define COLOR_VIOLET     0x6C71C4
#define COLOR_BLUE       0x268bD2
#define COLOR_CYAN       0x2AA198
#define COLOR_GREEN      0x859900
#define COLOR_BACKGROUND COLOR_BLACK

typedef enum color_t
{
    YELLOW,
    ORANGE,
    RED,
    MAGENTA,
    VIOLET,
    BLUE,
    CYAN,
    GREEN,
    NUM_COLORS,
} color_t;


struct OffscreenBuffer
{
    // NOTE(amin): pixels are always 32-bits wide. Memory order: BB GG RR XX.
    void *memory;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
};

void update(int buffer_width, int buffer_height, struct Star stars[], int num_stars, struct QuadTree *qt);
void render(struct OffscreenBuffer *buffer, float dt, struct Star stars[], int num_stars, struct QuadTree *qt);
void set_pixel(struct OffscreenBuffer *buffer, uint32_t x, uint32_t y, uint32_t color);
void draw_grid(struct OffscreenBuffer *buffer, struct QuadTreeNode *node, uint32_t color);

#define STAR_GARDEN_H
#endif
