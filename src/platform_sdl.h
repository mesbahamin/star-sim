#ifndef PLATFORM_SDL_H

#include "SDL.h"

struct SDLOffscreenBuffer
{
    // NOTE(amin): pixels are always 32-bits wide. Memory order: BB GG RR XX.
    SDL_Texture *texture;
    void *memory;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
};

struct SDLWindowDimension
{
    int width;
    int height;
};

#define PLATFORM_SDL_H
#endif
