#include "barnes_hut.h"

#include "star.h"

#include "math.h"
#include "stdlib.h"


struct Cell *cell_init(float center_x, float center_y, float distance_x, float distance_y)
{
    struct Cell *cell = (struct Cell *)malloc(sizeof(struct Cell));
    if (cell)
    {
        cell->center_x = center_x;
        cell->center_y = center_y;
        cell->distance_x = distance_x;
        cell->distance_y = distance_y;
        cell->mass_center_x = center_x;
        cell->mass_center_y = center_x;
        cell->mass_total = 0;
        cell->stars_sum_x = 0;
        cell->stars_sum_y = 0;
        cell->star = NULL;
    }
    return cell;
}


void cell_free(struct Cell *c)
{
    if (c)
    {
        free(c);
    }
}


bool cell_contains_star(struct Cell *c, struct Star *s)
{
    return (s->x <= c->center_x + c->distance_x
        && s->x >= c->center_x - c->distance_x
        && s->y <= c->center_y + c->distance_y
        && s->y >= c->center_y - c->distance_y);
}


bool cell_is_empty(struct Cell *c)
{
    return !(c->star);
}


struct QuadTreeNode *quad_tree_node_init(float center_x, float center_y, float distance_x, float distance_y)
{
    struct QuadTreeNode *node = (struct QuadTreeNode *)malloc(sizeof (struct QuadTreeNode));
    if (node)
    {
        node->cell = cell_init(center_x, center_y, distance_x, distance_y);
        node->ne = NULL;
        node->nw = NULL;
        node->sw = NULL;
        node->se = NULL;
    }
    return node;
}


void quad_tree_node_free(struct QuadTreeNode *node)
{
    if (node)
    {
        quad_tree_node_free(node->ne);
        quad_tree_node_free(node->nw);
        quad_tree_node_free(node->sw);
        quad_tree_node_free(node->se);
        free(node->cell);
        free(node);
    }
}


bool quad_tree_node_is_leaf(struct QuadTreeNode *node)
{
    if (!node)
    {
        return false;
    }
    return !(node->ne && node->nw && node->sw && node->se);
}


void quad_tree_node_subdivide(struct QuadTreeNode *node)
{
    if (node && quad_tree_node_is_leaf(node))
    {
        float child_distance_x = node->cell->distance_x / 2;
        float child_distance_y = node->cell->distance_y / 2;

        node->ne = quad_tree_node_init(node->cell->center_x + child_distance_x, node->cell->center_y + child_distance_y, child_distance_x, child_distance_y);
        node->nw = quad_tree_node_init(node->cell->center_x - child_distance_x, node->cell->center_y + child_distance_y, child_distance_x, child_distance_y);
        node->sw = quad_tree_node_init(node->cell->center_x - child_distance_x, node->cell->center_y - child_distance_y, child_distance_x, child_distance_y);
        node->se = quad_tree_node_init(node->cell->center_x + child_distance_x, node->cell->center_y - child_distance_y, child_distance_x, child_distance_y);
    }
}


void quad_tree_node_insert_star(struct QuadTreeNode *node, struct Star *star)
{
    if (node && cell_contains_star(node->cell, star))
    {
        if (quad_tree_node_is_leaf(node))
        {
            if (cell_is_empty(node->cell))
            {
                node->cell->star = star;
            }
            else
            {
                quad_tree_node_subdivide(node);

                quad_tree_node_insert_star(node->ne, node->cell->star);
                quad_tree_node_insert_star(node->nw, node->cell->star);
                quad_tree_node_insert_star(node->sw, node->cell->star);
                quad_tree_node_insert_star(node->se, node->cell->star);

                quad_tree_node_insert_star(node->ne, star);
                quad_tree_node_insert_star(node->nw, star);
                quad_tree_node_insert_star(node->sw, star);
                quad_tree_node_insert_star(node->se, star);

                node->cell->star = NULL;
            }
        }
        else
        {
            node->cell->mass_total += star->mass;
            node->cell->stars_sum_x += star->x * star->mass;
            node->cell->stars_sum_y += star->y * star->mass;

            float x_avg = node->cell->stars_sum_x / node->cell->mass_total;
            float y_avg = node->cell->stars_sum_y / node->cell->mass_total;

            node->cell->mass_center_x = x_avg;
            node->cell->mass_center_y = y_avg;

            quad_tree_node_insert_star(node->ne, star);
            quad_tree_node_insert_star(node->nw, star);
            quad_tree_node_insert_star(node->sw, star);
            quad_tree_node_insert_star(node->se, star);
        }
    }
}


struct QuadTree *quad_tree_init(void)
{
    struct QuadTree *qt = (struct QuadTree *)malloc(sizeof (struct QuadTree));
    if (qt)
    {
        qt->root = NULL;
    }
    return qt;
}


void quad_tree_free(struct QuadTree *qt)
{
    if (qt)
    {
        quad_tree_node_free(qt->root);
        free(qt);
    }
}


bool node_is_sufficiently_far(struct QuadTreeNode *node, struct Star *star)
{
    float s = node->cell->distance_x * 2;
    float dx = node->cell->mass_center_x - star->x;
    float dy = node->cell->mass_center_y - star->y;
    float d = hypotf(dx, dy);

    return s / d < THETA;
}


void quad_tree_calc_force_on_star(struct QuadTreeNode *node, struct Star *star)
{
    if (!star || !node)
    {
        return;
    }

    if (quad_tree_node_is_leaf(node) && !cell_is_empty(node->cell) && !cell_contains_star(node->cell, star))
    {
        star_attract(node->cell->star, star);
    }
    else if (node_is_sufficiently_far(node, star))
    {
        star_attract_to_mass(star,
                node->cell->mass_total,
                node->cell->mass_center_x,
                node->cell->mass_center_y);
    }
    else
    {
        quad_tree_calc_force_on_star(node->ne, star);
        quad_tree_calc_force_on_star(node->nw, star);
        quad_tree_calc_force_on_star(node->sw, star);
        quad_tree_calc_force_on_star(node->se, star);
    }
}
