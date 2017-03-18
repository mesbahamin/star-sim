#include <cmath>
#include <inttypes.h>
#include <SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#define TITLE "Stars"
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define BYTES_PER_PIXEL 4
#define NUM_PARTICLES 100000

#define COLOR_YELLOW     0xB58900
#define COLOR_ORANGE     0xCB4B16
#define COLOR_RED        0xDC322F
#define COLOR_MAGENTA    0xD33682
#define COLOR_VIOLET     0x6C71C4
#define COLOR_BLUE       0x268bD2
#define COLOR_CYAN       0x2AA198
#define COLOR_GREEN      0x859900
#define COLOR_BACKGROUND 0x002B36

#define SECOND 1000.0f
#define FPS 60
#define MS_PER_FRAME (SECOND / FPS)
#define UPDATES_PER_SECOND 120
#define MS_PER_UPDATE (SECOND / UPDATES_PER_SECOND)

enum color_t
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
};

struct SDLOffscreenBuffer
{
    // NOTE(amin): pixels are always 32-bits wide. Memory order: BB GG RR XX.
    SDL_Texture *texture;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct SDLWindowDimension
{
    int width;
    int height;
};

struct Particle
{
    float angle;
    float speed;
    float mass;
    double x;
    double y;
    uint32_t color;
};

static SDLOffscreenBuffer global_back_buffer;

uint64_t get_current_time_ms(void)
{
    struct timespec current;
    // TODO(amin): Fallback to other time sources when CLOCK_MONOTONIC is unavailable.
    clock_gettime(CLOCK_MONOTONIC, &current);
    uint64_t milliseconds = ((current.tv_sec * 1000000000) + current.tv_nsec) / 1000000;
    return milliseconds;
}


SDLWindowDimension sdl_get_window_dimension(SDL_Window *window)
{
    SDLWindowDimension result;
    SDL_GetWindowSize(window, &result.width, &result.height);
    return(result);
}


void sdl_resize_texture(SDLOffscreenBuffer *buffer, SDL_Renderer *renderer, int width, int height)
{
    if (buffer->memory)
    {
        munmap(buffer->memory, buffer->width * buffer->height * BYTES_PER_PIXEL);
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

    buffer->memory = mmap(
            0,
            width * height * BYTES_PER_PIXEL,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1, 0);
}


void sdl_update_window(SDL_Window *window, SDL_Renderer *renderer, SDLOffscreenBuffer buffer)
{
    if (SDL_UpdateTexture(buffer.texture, 0, buffer.memory, buffer.pitch))
    {
        // TODO(amin): Handle this error
    }

    SDL_RenderCopy(renderer, buffer.texture, 0, 0);
    SDL_RenderPresent(renderer);
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
                    sdl_update_window(window, renderer, global_back_buffer);
                } break;
            }
        } break;
    }
    return(should_quit);
}


// TODO: change buffer to a pointer
void set_pixel(SDLOffscreenBuffer buffer, uint32_t x, uint32_t y, uint32_t color)
{
    /* Origin is (0, 0) on the upper left.
     * To go one pixel right, increment by 32 bits.
     * To go one pixel down, increment by (buffer.width * 32) bits.
     */
    if (x < buffer.width && y < buffer.height)
    {
        uint8_t *pixel_pos = (uint8_t *)buffer.memory;
        pixel_pos += ((BYTES_PER_PIXEL*x) + (buffer.pitch * y));
        uint32_t *pixel = (uint32_t *)pixel_pos;
        *pixel = color;
    }
}

// TODO: change buffer to a pointer
void clear_screen(SDLOffscreenBuffer buffer, uint32_t color)
{
    uint8_t *row = (uint8_t *)buffer.memory;

    for (int y = 0; y < buffer.height; ++y)
    {
        uint32_t *pixel = (uint32_t *)row;

        for (int x = 0; x < buffer.width; ++x)
        {

            *pixel++ = color;
        }

        row += buffer.pitch;
    }
}

void update(Particle particles[], int num_particles, SDLWindowDimension* dimension)
{
    for (int i = 0; i < num_particles; ++i)
    {
        particles[i].x += sinf(particles[i].angle) * particles[i].speed;
        particles[i].y += cosf(particles[i].angle) * particles[i].speed;

        if (particles[i].x > dimension->width)
        {
            particles[i].x = 0;
        }
        else if (particles[i].x < 0)
        {
            particles[i].x = dimension->width;
        }

        if (particles[i].y > dimension->height)
        {
            particles[i].y = 0;
        }
        else if (particles[i].y < 0)
        {
            particles[i].y = dimension->height;
        }
    }
}

void render(SDLOffscreenBuffer buffer, float dt, Particle particles[], int num_particles)
{
    //printf("%f\n", dt);
    for (int i = 0; i < num_particles; ++i)
    {
        set_pixel(
            buffer,
            particles[i].x + (sinf(particles[i].angle) * particles[i].speed) * dt,
            particles[i].y + (cosf(particles[i].angle) * particles[i].speed) * dt,
            particles[i].color);
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
            0);

    if (window)
    {
        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

        if (renderer)
        {
            SDLWindowDimension dimension = sdl_get_window_dimension(window);
            sdl_resize_texture(&global_back_buffer, renderer, dimension.width, dimension.height);

            bool running = true;
            srand((uint32_t)time(NULL));

            uint64_t lag = 0;
            uint64_t previous_ms = get_current_time_ms();
            Particle particles[NUM_PARTICLES];
            for (int i = 0; i < NUM_PARTICLES; ++i)
            {
                particles[i].x = rand() % dimension.width;
                particles[i].y = rand() % dimension.height;
                particles[i].angle = ((float)rand()/(float)(RAND_MAX)) * 2 * M_PI;
                particles[i].speed = 1;
                particles[i].mass = 10;

                enum color_t color = (color_t)(rand() % NUM_COLORS);
                switch(color)
                {
                    case YELLOW:
                        particles[i].color = COLOR_YELLOW;
                        break;
                    case ORANGE:
                        particles[i].color = COLOR_ORANGE;
                        break;
                    case RED:
                        particles[i].color = COLOR_RED;
                        break;
                    case MAGENTA:
                        particles[i].color = COLOR_MAGENTA;
                        break;
                    case VIOLET:
                        particles[i].color = COLOR_VIOLET;
                        break;
                    case BLUE:
                        particles[i].color = COLOR_BLUE;
                        break;
                    case CYAN:
                        particles[i].color = COLOR_CYAN;
                        break;
                    case GREEN:
                        particles[i].color = COLOR_GREEN;
                        break;
                    default:
                        particles[i].color = 0xFFFFFF;
                        break;
                }
                //printf("%f, %f, %f, %f, %f\n", particles[i].angle, sinf(particles[i].angle), cosf(particles[i].angle), particles[i].speed, particles[i].mass);
            }

            while (running)
            {
                clear_screen(global_back_buffer, COLOR_BACKGROUND);
                uint64_t current_ms = get_current_time_ms();
                uint64_t elapsed_ms = current_ms - previous_ms;
                previous_ms = current_ms;
                lag += elapsed_ms;
                printf("Lag: %d\n", lag);

                printf("%" PRIu64 ", %f\n", lag, MS_PER_UPDATE);
                SDL_Event event;

                while (SDL_PollEvent(&event))
                {
                    running = !handle_event(&event);
                }

                SDL_PumpEvents();

                dimension = sdl_get_window_dimension(window);

                while (lag >= MS_PER_UPDATE)
                {
                    update(particles, NUM_PARTICLES, &dimension);
                    printf("\t%" PRIu64 ", %f\n", lag, MS_PER_UPDATE);
                    lag -= MS_PER_UPDATE;
                }
                render(global_back_buffer, lag/SECOND, particles, NUM_PARTICLES);
                sdl_update_window(window, renderer, global_back_buffer);
                if (elapsed_ms <= MS_PER_FRAME)
                {
                    usleep((MS_PER_FRAME - elapsed_ms) * SECOND);
                }
            }
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