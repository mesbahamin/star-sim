#include "star_garden.h"

#include <stdbool.h>

bool PAUSED = false;
bool BRUTE_FORCE = false;
bool RENDER_GRID = false;
bool RENDER_TRAILS = false;


void sim_init(struct SimState *sim_state, int field_width, int field_height)
{
    if (!sim_state)
    {
        // TODO: handle invalid pointer error
        return;
    }

    sim_state->num_stars = NUM_STARS;
    for (int i = 0; i < sim_state->num_stars; ++i)
    {
        struct Star *star = &sim_state->stars[i];
        star->x = rand() % field_width;
        star->y = rand() % field_height;
        star->angle = ((float)rand()/(float)(RAND_MAX)) * 2 * M_PI;
        star->speed = 0;
        star->mass = 5;
        star->size = star_calc_size(star->mass);
        star->color = COLOR_WHITE;
        //printf("%f, %f, %f\n", star->angle, star->speed, star->mass);
    }
    sim_state->qt = quad_tree_init();
}


void sim_update(struct SimState *sim_state, int field_width, int field_height)
{
    if (!sim_state)
    {
        // TODO: handle invalid pointer error
        return;
    }

    // TODO: either limit the bounds of the simulation, or base these values on
    // the smallest bounding rectangle
    int center_x = field_width / 2;
    int center_y = field_height / 2;
    int dist_x = field_width / 2;
    int dist_y = field_height / 2;

    int num_stars = sim_state->num_stars;
    struct Star *stars = sim_state->stars;
    struct QuadTree *qt = sim_state->qt;

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
        if (stars[i].x < 0 || stars[i].x > field_width
            || stars[i].y < 0 || stars[i].y > field_height)
        {
            stars[i].x = rand() % field_width;
            stars[i].y = rand() % field_height;
            stars[i].angle = 0;
            stars[i].speed = 0;
        }
#endif
    }
}


void sim_render(struct OffscreenBuffer *buffer, float dt, struct SimState *sim_state)
{
    if (!buffer || !sim_state)
    {
        // TODO: handle invalid pointer error
        return;
    }

    int num_stars = sim_state->num_stars;
    struct Star *stars = sim_state->stars;
    struct QuadTree *qt = sim_state->qt;
    //printf("%f\n", dt);
    if (BRUTE_FORCE)
    {
        for (int i = 0; i < num_stars; ++i)
        {
            sim_set_pixel(
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
            sim_set_pixel(
                buffer,
                stars[i].x + (sinf(stars[i].angle) * stars[i].speed) * dt,
                stars[i].y - (cosf(stars[i].angle) * stars[i].speed) * dt,
                stars[i].color);
        }
    }

    if (RENDER_GRID)
    {
        sim_render_grid(buffer, qt->root, COLOR_GREEN);
    }
}


void sim_set_pixel(struct OffscreenBuffer *buffer, uint32_t x, uint32_t y, uint32_t color)
{
    if (!buffer)
    {
        // TODO: handle invalid pointer error
        return;
    }

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


void sim_render_grid(struct OffscreenBuffer *buffer, struct QuadTreeNode *node, uint32_t color)
{
    if (!buffer)
    {
        // TODO: handle invalid pointer error
        return;
    }

    if (node && node->ne && node->nw && node->sw && node->se)
    {
        for (int x = (node->cell->center_x - node->cell->distance_x);
            x <= (node->cell->center_x + node->cell->distance_x);
            ++x)
        {
            sim_set_pixel(buffer, x, node->cell->center_y, color);
        }

        for (int y = (node->cell->center_y - node->cell->distance_y);
            y <= (node->cell->center_y + node->cell->distance_y);
            ++y)
        {
            sim_set_pixel(buffer, node->cell->center_x, y, color);
        }

        sim_render_grid(buffer, node->ne, color);
        sim_render_grid(buffer, node->nw, color);
        sim_render_grid(buffer, node->sw, color);
        sim_render_grid(buffer, node->se, color);
    }
}


void sim_cleanup(struct SimState *sim_state)
{
    if (!sim_state)
    {
        // TODO: handle invalid pointer error
        return;
    }

    quad_tree_free(sim_state->qt);
}
