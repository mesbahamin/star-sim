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

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef M_PI
#define M_PI (3.14159265358979323846264338327950288)
#endif

#define FULLSCREEN
#define RESPAWN_STARS
#define USE_TEST_SEED
bool BRUTE_FORCE = false;
bool RENDER_GRID = false;
bool RENDER_TRAILS = false;
bool PAUSED = false;

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


void set_pixel(struct SDLOffscreenBuffer *buffer, uint32_t x, uint32_t y, uint32_t color)
{
    /* Origin is (0, 0) on the upper left.
     * To go one pixel right, increment by 32 bits.
     * To go one pixel down, increment by (buffer.width * 32) bits.
     */
    if (x < buffer->width && y < buffer->height)
    {
        uint8_t *pixel_pos = (uint8_t *)buffer->memory;
        pixel_pos += ((BYTES_PER_PIXEL*x) + (buffer->pitch * y));
        uint32_t *pixel = (uint32_t *)pixel_pos;
        *pixel = color;
    }
}


void update(struct Star stars[], int num_stars, struct QuadTree *qt)
{
    // TODO: either limit the bounds of the simulation, or base these values on
    // the smallest bounding rectangle
    int center_x = global_back_buffer.width / 2;
    int center_y = global_back_buffer.height / 2;
    int dist_x = global_back_buffer.width / 2;
    int dist_y = global_back_buffer.height / 2;

    if (BRUTE_FORCE)
    {
        for (int i = 0; i < num_stars; ++i)
        {
            for (int j = i + 1; j < num_stars; ++j)
            {
                star_attract(&stars[i], &stars[j]);
            }
        }
    }
    else
    {
        quad_tree_node_free(qt->root);
        qt->root = quad_tree_node_init(center_x, center_y, dist_x, dist_y);
        for (int i = 0; i < num_stars; ++i)
        {
            quad_tree_node_insert_star(qt->root, &(stars[i]));
        }
        for (int i = 0; i < num_stars; ++i)
        {
            // TODO: Will this result in stars being attracted to each other
            // twice?
            quad_tree_calc_force_on_star(qt->root, &(stars[i]));
        }
    }
    for (int i = 0; i < num_stars; ++i)
    {
        stars[i].x += sinf(stars[i].angle) * stars[i].speed;
        stars[i].y -= cosf(stars[i].angle) * stars[i].speed;
        stars[i].speed *= DRAG;
#ifdef RESPAWN_STARS
        if (stars[i].x < 0 || stars[i].x > global_back_buffer.width
            || stars[i].y < 0 || stars[i].y > global_back_buffer.height)
        {
            stars[i].x = rand() % global_back_buffer.width;
            stars[i].y = rand() % global_back_buffer.height;
            stars[i].angle = 0;
            stars[i].speed = 0;
        }
#endif
    }
}


void draw_grid(struct SDLOffscreenBuffer *buffer, struct QuadTreeNode *node, uint32_t color)
{
    if (node && node->ne && node->nw && node->sw && node->se)
    {
        for (int x = (node->cell->center_x - node->cell->distance_x);
            x <= (node->cell->center_x + node->cell->distance_x);
            ++x)
        {
            set_pixel(buffer, x, node->cell->center_y, color);
        }

        for (int y = (node->cell->center_y - node->cell->distance_y);
            y <= (node->cell->center_y + node->cell->distance_y);
            ++y)
        {
            set_pixel(buffer, node->cell->center_x, y, color);
        }

        draw_grid(buffer, node->ne, color);
        draw_grid(buffer, node->nw, color);
        draw_grid(buffer, node->sw, color);
        draw_grid(buffer, node->se, color);
    }
}


void render(struct SDLOffscreenBuffer *buffer, float dt, struct Star stars[], int num_stars, struct QuadTree *qt)
{
    //printf("%f\n", dt);
    if (BRUTE_FORCE)
    {
        for (int i = 0; i < num_stars; ++i)
        {
            set_pixel(
                buffer,
                stars[i].x + (sinf(stars[i].angle) * stars[i].speed) * dt,
                stars[i].y - (cosf(stars[i].angle) * stars[i].speed) * dt,
                COLOR_CYAN);
        }
    }
    else
    {
        for (int i = 0; i < num_stars; ++i)
        {
            set_pixel(
                buffer,
                stars[i].x + (sinf(stars[i].angle) * stars[i].speed) * dt,
                stars[i].y - (cosf(stars[i].angle) * stars[i].speed) * dt,
                stars[i].color);
        }
    }

    if (RENDER_GRID)
    {
        draw_grid(buffer, qt->root, COLOR_GREEN);
    }
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

                if (PAUSED)
                {
                    lag = 0;
                }
                else
                {
                    while (lag >= MS_PER_UPDATE)
                    {
                        update(stars, NUM_STARS, qt);
                        //printf("\t%" PRIu64 ", %f\n", lag, MS_PER_UPDATE);
                        lag -= MS_PER_UPDATE;
                    }
                }

                if (!RENDER_TRAILS)
                {
                    clear_screen(&global_back_buffer, COLOR_BACKGROUND);
                }
                render(&global_back_buffer, lag/SECOND, stars, NUM_STARS, qt);
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
