#include "platform_sdl.h"

#include "sim.h"

#include <stdbool.h>
#include <stdio.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

extern bool PAUSED;
extern bool BRUTE_FORCE;
extern bool RENDER_GRID;
extern bool RENDER_BOUNDING_BOX;
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
                    RENDER_BOUNDING_BOX = !RENDER_BOUNDING_BOX;
                }
                if (key_code == SDLK_f)
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


int main(int argc, char *argv[])
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

            struct SimState sim_state;
            sim_init(&sim_state, dimension.width, dimension.height);

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

                const uint8_t *keystate = SDL_GetKeyboardState(0);

                // TODO: move this to a function
                if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_H])
                {
                    sim_state.view.dx += 5 / sim_state.view.zoom;
                }
                if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_L])
                {
                    sim_state.view.dx -= 5 / sim_state.view.zoom;
                }
                if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_K])
                {
                    sim_state.view.dy += 5 / sim_state.view.zoom;
                }
                if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_J])
                {
                    sim_state.view.dy -= 5 / sim_state.view.zoom;
                }
                if (keystate[SDL_SCANCODE_LEFT])
                {
                    sim_state.view.dx += 1 / sim_state.view.zoom;
                }
                if (keystate[SDL_SCANCODE_RIGHT])
                {
                    sim_state.view.dx -= 1 / sim_state.view.zoom;
                }
                if (keystate[SDL_SCANCODE_UP])
                {
                    sim_state.view.dy += 1 / sim_state.view.zoom;
                }
                if (keystate[SDL_SCANCODE_DOWN])
                {
                    sim_state.view.dy -= 1 / sim_state.view.zoom;
                }
                if (keystate[SDL_SCANCODE_EQUALS])
                {
                    sim_state.view.zoom *= 1.01;
                }
                if (keystate[SDL_SCANCODE_MINUS])
                {
                    sim_state.view.zoom *= 0.99;
                }
                if (keystate[SDL_SCANCODE_0])
                {
                    sim_state.view.zoom = 1;
                }
                if (keystate[SDL_SCANCODE_HOME])
                {
                    sim_state.view.dx = 0;
                    sim_state.view.dy = 0;
                    sim_state.view.zoom = 1;
                }

                struct OffscreenBuffer buffer;
                // WARNING: these pointers are aliased until the end of the
                // loop
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
                        sim_update(&sim_state, buffer.width, buffer.height);
                        //printf("\t%" PRIu64 ", %f\n", lag, MS_PER_UPDATE);
                        lag -= MS_PER_UPDATE;
                    }
                }

                if (!RENDER_TRAILS)
                {
                    clear_screen(&global_back_buffer, COLOR_BACKGROUND);
                }

                sim_render(&buffer, lag/SECOND, &sim_state);
                sdl_update_window(renderer, &global_back_buffer);
                if (elapsed_ms <= MS_PER_FRAME)
                {
                    SDL_Delay(MS_PER_FRAME - elapsed_ms);
                }
            }

            sim_cleanup(&sim_state);
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
