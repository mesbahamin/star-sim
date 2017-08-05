#include "star_garden.h"

#include <stdbool.h>

bool PAUSED = false;
bool BRUTE_FORCE = false;
bool RENDER_GRID = false;
bool RENDER_BOUNDING_BOX = false;
bool RENDER_TRAILS = false;


void sim_init(struct SimState *sim_state, int field_width, int field_height)
{
    if (!sim_state)
    {
        // TODO: handle invalid pointer error
        return;
    }

    sim_state->num_stars = NUM_STARS;

    sim_state->bounding_box.center_x = field_width / 2;
    sim_state->bounding_box.center_y = field_height / 2;
    sim_state->bounding_box.side_length_x = 0;
    sim_state->bounding_box.side_length_y = 0;

    sim_state->view.dx = 0;
    sim_state->view.dy = 0;
    sim_state->view.zoom = 1;

    float min_x = field_width / 2;
    float max_x = field_width / 2;
    float min_y = field_height / 2;
    float max_y = field_height / 2;

    for (int i = 0; i < sim_state->num_stars; ++i)
    {
        int star_x = rand() % field_width;
        int star_y = rand() % field_height;

        struct Star *star = &sim_state->stars[i];
        star->x = star_x;
        star->y = star_y;
        star->angle = ((float)rand()/(float)(RAND_MAX)) * 2 * M_PI;
        star->speed = 0;
        star->mass = 5;
        star->size = star_calc_size(star->mass);
        star->color = COLOR_WHITE;
        //printf("%f, %f, %f\n", star->angle, star->speed, star->mass);

        if (star_x <= min_x)
        {
            min_x = star_x;
        }
        else if (star_x >= max_x)
        {
            max_x = star_x;
        }
        if (star_y <= min_y)
        {
            min_y = star_y;
        }
        else if (star_y >= max_y)
        {
            max_y = star_y;
        }
    }
    sim_bounding_box_update(&sim_state->bounding_box, min_x, min_y, max_x, max_y);

    sim_state->qt = quad_tree_init();
}


void sim_bounding_box_update(struct SimBounds *bounds, float min_x, float min_y, float max_x, float max_y)
{
    if (!bounds)
    {
        // TODO: handle invalid pointer error
        return;
    }

    float bounding_square_side_length = fmaxf(max_x - min_x, max_y - min_y);
    bounds->side_length_x = bounding_square_side_length;
    bounds->side_length_y = bounding_square_side_length;
    bounds->center_x = (min_x + max_x) / 2;
    bounds->center_y = (min_y + max_y) / 2;
}


void sim_update(struct SimState *sim_state, int field_width, int field_height)
{
    if (!sim_state)
    {
        // TODO: handle invalid pointer error
        return;
    }

    float min_x = sim_state->bounding_box.center_x;
    float max_x = sim_state->bounding_box.center_x;
    float min_y = sim_state->bounding_box.center_y;
    float max_y = sim_state->bounding_box.center_y;

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
        qt->root = quad_tree_node_init(
                sim_state->bounding_box.center_x,
                sim_state->bounding_box.center_y,
                sim_state->bounding_box.side_length_x / 2,
                sim_state->bounding_box.side_length_y / 2);

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

        if (stars[i].x <= min_x)
        {
            min_x = stars[i].x;
        }
        else if (stars[i].x >= max_x)
        {
            max_x = stars[i].x;
        }
        if (stars[i].y <= min_y)
        {
            min_y = stars[i].y;
        }
        else if (stars[i].y >= max_y)
        {
            max_y = stars[i].y;
        }
    }

    sim_bounding_box_update(&sim_state->bounding_box, min_x, min_y, max_x, max_y);
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

    float dx = sim_state->view.dx;
    float dy = sim_state->view.dy;
    float zoom = sim_state->view.zoom;

    for (int i = 0; i < num_stars; ++i)
    {
        uint32_t color;
        if (BRUTE_FORCE)
        {
            color = COLOR_CYAN;
        }
        else
        {
            color = stars[i].color;
        }
        sim_set_pixel(
            buffer,
            sim_calc_render_offset(zoom, dx, stars[i].x + (sinf(stars[i].angle) * stars[i].speed) * dt, buffer->width/2),
            sim_calc_render_offset(zoom, dy, stars[i].y - (cosf(stars[i].angle) * stars[i].speed) * dt, buffer->height/2),
            color);
    }

    if (RENDER_GRID)
    {
        sim_grid_render(buffer, qt->root, COLOR_GREEN, &sim_state->view);
    }
    if (RENDER_BOUNDING_BOX)
    {
        sim_bounding_box_render(buffer, &sim_state->bounding_box, COLOR_RED, &sim_state->view);
    }
}


void sim_grid_render(struct OffscreenBuffer *buffer, struct QuadTreeNode *node, uint32_t color, struct SimView *view)
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
            sim_set_pixel(
                    buffer,
                    sim_calc_render_offset(view->zoom, view->dx, x, buffer->width/2),
                    sim_calc_render_offset(view->zoom, view->dy, node->cell->center_y, buffer->height/2),
                    color);
        }

        for (int y = (node->cell->center_y - node->cell->distance_y);
            y <= (node->cell->center_y + node->cell->distance_y);
            ++y)
        {
            sim_set_pixel(
                    buffer,
                    sim_calc_render_offset(view->zoom, view->dx, node->cell->center_x, buffer->width/2),
                    sim_calc_render_offset(view->zoom, view->dy, y, buffer->height/2),
                    color);
        }

        sim_grid_render(buffer, node->ne, color, view);
        sim_grid_render(buffer, node->nw, color, view);
        sim_grid_render(buffer, node->sw, color, view);
        sim_grid_render(buffer, node->se, color, view);
    }
}


void sim_bounding_box_render(struct OffscreenBuffer *buffer, struct SimBounds *bounding_box, uint32_t color, struct SimView *view)
{
    if (!buffer)
    {
        // TODO: handle invalid pointer error
        return;
    }

    if (bounding_box)
    {
        int reticle_radius = 3;
        for (int x = bounding_box->center_x - reticle_radius;
            x <= bounding_box->center_x + reticle_radius;
            x++)
        {
            sim_set_pixel(
                    buffer,
                    sim_calc_render_offset(view->zoom, view->dx, x, buffer->width/2),
                    sim_calc_render_offset(view->zoom, view->dy, bounding_box->center_y, buffer->height/2),
                    color);
        }
        for (int y = bounding_box->center_y - reticle_radius;
            y <= bounding_box->center_y + reticle_radius;
            y++)
        {
            sim_set_pixel(
                    buffer,
                    sim_calc_render_offset(view->zoom, view->dx, bounding_box->center_x, buffer->width/2),
                    sim_calc_render_offset(view->zoom, view->dy, y, buffer->height/2),
                    color);
        }

        float half_length_x = bounding_box->side_length_x / 2;
        float half_length_y = bounding_box->side_length_y / 2;

        for (int x = (bounding_box->center_x - half_length_x);
            x <= (bounding_box->center_x + half_length_x);
            ++x)
        {
            sim_set_pixel(
                    buffer,
                    sim_calc_render_offset(view->zoom, view->dx, x, buffer->width/2),
                    sim_calc_render_offset(view->zoom, view->dy, bounding_box->center_y - half_length_y, buffer->height/2),
                    color);
            sim_set_pixel(
                    buffer,
                    sim_calc_render_offset(view->zoom, view->dx, x, buffer->width/2),
                    sim_calc_render_offset(view->zoom, view->dy, bounding_box->center_y + half_length_y, buffer->height/2),
                    color);
        }

        for (int y = (bounding_box->center_y - half_length_y);
            y <= (bounding_box->center_y + half_length_y);
            ++y)
        {
            sim_set_pixel(
                    buffer,
                    sim_calc_render_offset(view->zoom, view->dx, bounding_box->center_x - half_length_x, buffer->width/2),
                    sim_calc_render_offset(view->zoom, view->dy, y, buffer->height/2),
                    color);
            sim_set_pixel(
                    buffer,
                    sim_calc_render_offset(view->zoom, view->dx, bounding_box->center_x + half_length_x, buffer->width/2),
                    sim_calc_render_offset(view->zoom, view->dy, y, buffer->height/2),
                    color);
        }
    }
}


float sim_calc_render_offset(float zoom, float delta, float pos, float center)
{
    return ((1 - zoom) * center) + (pos + delta) * zoom;
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


void sim_cleanup(struct SimState *sim_state)
{
    if (!sim_state)
    {
        // TODO: handle invalid pointer error
        return;
    }

    quad_tree_free(sim_state->qt);
}
