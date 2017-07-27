#include "star_garden.h"

#include <inttypes.h>
#include <math.h>
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform_sdl.h"

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif

extern bool PAUSED;
extern bool BRUTE_FORCE;
extern bool RENDER_GRID;
extern bool RENDER_TRAILS;

static struct SDLOffscreenBuffer global_back_buffer;


struct SDLWindowDimension sdl_get_window_dimension(SDL_Window *window)
{
    struct SDLWindowDimension result;
    SDL_GetWindowSize(window, &result.width, &result.height);
    return(result);
}


void sdl_resize_texture(struct SDLOffscreenBuffer *buffer, SDL_Renderer *renderer, int width, int height)
{
    if (buffer->memory)
    {
        free(buffer->memory);
    }

    if (buffer->texture)
    {
        SDL_DestroyTexture(buffer->texture);
    }

    buffer->texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            width, height);

    buffer->width = width;
    buffer->height = height;
    buffer->pitch = width * BYTES_PER_PIXEL;

    buffer->memory = malloc(width * height * BYTES_PER_PIXEL);
}


void sdl_update_window(SDL_Renderer *renderer, struct SDLOffscreenBuffer *buffer)
{
    if (SDL_UpdateTexture(buffer->texture, 0, buffer->memory, buffer->pitch))
    {
        // TODO(amin): Handle this error
    }

    SDL_RenderCopy(renderer, buffer->texture, 0, 0);
    SDL_RenderPresent(renderer);
}


void clear_screen(struct SDLOffscreenBuffer *buffer, uint32_t pixel_value)
{
    // NOTE(amin): Memset is faster than nested for loops, but can only set
    // pixels to single byte values
    memset(buffer->memory, pixel_value, buffer->height * buffer->width * BYTES_PER_PIXEL);
}


bool handle_event(SDL_Event *event)
{
    bool should_quit = false;

    switch(event->type)
    {
        case SDL_QUIT:
        {
            printf("SDL_QUIT\n");
            should_quit = true;
        } break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            SDL_Keycode key_code = event->key.keysym.sym;
            bool is_down = (event->key.state == SDL_PRESSED);
            if (is_down)
            {
                if (key_code == SDLK_b)
                {
                    BRUTE_FORCE = !BRUTE_FORCE;
                }
                if (key_code == SDLK_g)
                {
                    RENDER_GRID = !RENDER_GRID;
                }
                if (key_code == SDLK_p)
                {
                    PAUSED = !PAUSED;
                }
                if (key_code == SDLK_t)
                {
                    RENDER_TRAILS = !RENDER_TRAILS;
                    clear_screen(&global_back_buffer, COLOR_BACKGROUND);
                }
            }
        } break;
        case SDL_WINDOWEVENT:
        {
            switch(event->window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);
                    printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n", event->window.data1, event->window.data2);
                    sdl_resize_texture(&global_back_buffer, renderer, event->window.data1, event->window.data2);
                } break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    printf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
                } break;

                case SDL_WINDOWEVENT_EXPOSED:
                {
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);
                    sdl_update_window(renderer, &global_back_buffer);
                } break;
            }
        } break;
    }
    return(should_quit);
}


int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        // TODO(amin): log SDL_Init error
    }

    SDL_Window *window = SDL_CreateWindow(
            TITLE,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

#ifdef FULLSCREEN
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
#endif

    if (window)
    {
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

        if (renderer)
        {
            struct SDLWindowDimension dimension = sdl_get_window_dimension(window);
            sdl_resize_texture(&global_back_buffer, renderer, dimension.width, dimension.height);

            bool running = true;

#ifdef USE_TEST_SEED
            srand((uint32_t)0);
#else
            srand((uint32_t)time(NULL));
#endif

            uint64_t lag = 0;
            uint64_t previous_ms = (SDL_GetPerformanceCounter() * SECOND) / SDL_GetPerformanceFrequency();
            struct Star stars[NUM_STARS];
            for (int i = 0; i < NUM_STARS; ++i)
            {
                stars[i].x = rand() % dimension.width;
                stars[i].y = rand() % dimension.height;
                stars[i].angle = ((float)rand()/(float)(RAND_MAX)) * 2 * M_PI;
                stars[i].speed = 0;
                stars[i].mass = 5;
                stars[i].size = star_calc_size(stars[i].mass);
                stars[i].color = COLOR_WHITE;
                //printf("%f, %f, %f\n", stars[i].angle, stars[i].speed, stars[i].mass);
            }

            struct QuadTree *qt = quad_tree_init();

            while (running)
            {
                uint64_t current_ms = (SDL_GetPerformanceCounter() * SECOND) / SDL_GetPerformanceFrequency();
                uint64_t elapsed_ms = current_ms - previous_ms;
                previous_ms = current_ms;
                lag += elapsed_ms;
                //printf("%" PRIu64 ", %" PRIu64 ", %f\n", elapsed_ms, lag, MS_PER_UPDATE);

                SDL_Event event;

                while (SDL_PollEvent(&event))
                {
                    running = !handle_event(&event);
                }

                SDL_PumpEvents();

                dimension = sdl_get_window_dimension(window);
                struct OffscreenBuffer buffer;
                // these pointers are now aliased
                buffer.memory = global_back_buffer.memory;
                buffer.width = global_back_buffer.width;
                buffer.height = global_back_buffer.height;
                buffer.pitch = global_back_buffer.pitch;

                if (PAUSED)
                {
                    lag = 0;
                }
                else
                {
                    while (lag >= MS_PER_UPDATE)
                    {
                        update(buffer.width, buffer.height, stars, NUM_STARS, qt);
                        //printf("\t%" PRIu64 ", %f\n", lag, MS_PER_UPDATE);
                        lag -= MS_PER_UPDATE;
                    }
                }

                if (!RENDER_TRAILS)
                {
                    clear_screen(&global_back_buffer, COLOR_BACKGROUND);
                }

                render(&buffer, lag/SECOND, stars, NUM_STARS, qt);
                sdl_update_window(renderer, &global_back_buffer);
                if (elapsed_ms <= MS_PER_FRAME)
                {
                    SDL_Delay(MS_PER_FRAME - elapsed_ms);
                }
            }

            quad_tree_free(qt);
        }
        else
        {
            // TODO(amin): log SDL_Renderer error
        }
    }
    else
    {
        // TODO(amin): log SDL_Window error
    }

    SDL_Quit();
    return(0);
}
