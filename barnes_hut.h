/* Quadtree implementation to keep track of recursive subdivisions of the
 * simulation field into rectangular cells.
 */

#ifndef BARNES_HUT_H
#define BARNES_HUT_H

// TODO: Remove this.
#include "star.h"

#define MAX_STARS_PER_CELL 10
// TODO: Limit tree depth

struct Cell
{
    int num_stars;
    float center_x;
    float center_y;
    float distance_x;
    float distance_y;
    // TODO: Make this a void*.
    struct Star *stars[MAX_STARS_PER_CELL];
};

struct QuadTreeNode
{
    struct Cell *cell;
    struct QuadTreeNode *ne;
    struct QuadTreeNode *nw;
    struct QuadTreeNode *sw;
    struct QuadTreeNode *se;
};

struct QuadTree
{
    struct QuadTreeNode *root;
};


struct Cell *cell_init(float center_x, float center_y, float distance_x, float distance_y)
{
    struct Cell *cell = (struct Cell *)malloc(sizeof(struct Cell));
    if (cell)
    {
        cell->center_x = center_x;
        cell->center_y = center_y;
        cell->distance_x = distance_x;
        cell->distance_y = distance_y;
        cell->num_stars = 0;
        for (int i = 0; i < MAX_STARS_PER_CELL; ++i)
        {
            cell->stars[i] = NULL;
        }
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


bool cell_insert_star(struct Cell *c, struct Star *s)
{
    bool star_inserted = false;
    if (c)
    {
        if (c->num_stars < MAX_STARS_PER_CELL)
        {
            c->stars[c->num_stars] = s;
            c->num_stars += 1;
            star_inserted = true;
        }
    }
    return star_inserted;
}


int cell_count_stars_contained(struct Cell *c, struct Star stars[], int num_stars)
{
    int count = 0;
    for (int i = 0; i < num_stars; ++i)
    {
        if (cell_contains_star(c, &(stars[i])))
        {
            count++;
        }
    }
    return count;
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


void quad_tree_node_insert_star(struct QuadTreeNode *node, struct Star *star)
{
    if (node && cell_contains_star(node->cell, star))
    {
        bool inserted = cell_insert_star(node->cell, star);
        if (!inserted)
        {
            if (quad_tree_node_is_leaf(node))
            {
                float child_distance_x = node->cell->distance_x / 2;
                float child_distance_y = node->cell->distance_y / 2;

                node->ne = quad_tree_node_init(node->cell->center_x + child_distance_x, node->cell->center_y + child_distance_y, child_distance_x, child_distance_y);
                node->nw = quad_tree_node_init(node->cell->center_x - child_distance_x, node->cell->center_y + child_distance_y, child_distance_x, child_distance_y);
                node->sw = quad_tree_node_init(node->cell->center_x - child_distance_x, node->cell->center_y - child_distance_y, child_distance_x, child_distance_y);
                node->se = quad_tree_node_init(node->cell->center_x + child_distance_x, node->cell->center_y - child_distance_y, child_distance_x, child_distance_y);

                for (int i = 0; i < node->cell->num_stars; ++i)
                {
                    quad_tree_node_insert_star(node->ne, node->cell->stars[i]);
                    quad_tree_node_insert_star(node->nw, node->cell->stars[i]);
                    quad_tree_node_insert_star(node->sw, node->cell->stars[i]);
                    quad_tree_node_insert_star(node->se, node->cell->stars[i]);
                    node->cell->stars[i] = NULL;
                }
            }

            quad_tree_node_insert_star(node->ne, star);
            quad_tree_node_insert_star(node->nw, star);
            quad_tree_node_insert_star(node->sw, star);
            quad_tree_node_insert_star(node->se, star);
        }
    }
}


// TODO: Get rid of this
void quad_tree_node_subdivide(struct QuadTreeNode *node, struct Star stars[], int num_stars)
{
    if (node && cell_count_stars_contained(node->cell, stars, num_stars) > MAX_STARS_PER_CELL)
    {
        float child_distance_x = node->cell->distance_x / 2;
        float child_distance_y = node->cell->distance_y / 2;

        node->ne = quad_tree_node_init(node->cell->center_x + child_distance_x, node->cell->center_y + child_distance_y, child_distance_x, child_distance_y);
        node->nw = quad_tree_node_init(node->cell->center_x - child_distance_x, node->cell->center_y + child_distance_y, child_distance_x, child_distance_y);
        node->sw = quad_tree_node_init(node->cell->center_x - child_distance_x, node->cell->center_y - child_distance_y, child_distance_x, child_distance_y);
        node->se = quad_tree_node_init(node->cell->center_x + child_distance_x, node->cell->center_y - child_distance_y, child_distance_x, child_distance_y);

        quad_tree_node_subdivide(node->ne, stars, num_stars);
        quad_tree_node_subdivide(node->nw, stars, num_stars);
        quad_tree_node_subdivide(node->sw, stars, num_stars);
        quad_tree_node_subdivide(node->se, stars, num_stars);
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


void quad_tree_set_virtual_stars(struct QuadTreeNode *node, struct Star virtual_stars[], int *current_num, int max_virtual_stars)
{
    if (!node)
    {
        return;
    }

    if (!quad_tree_node_is_leaf(node))
    {
        quad_tree_set_virtual_stars(node->ne, virtual_stars, current_num, max_virtual_stars);
        quad_tree_set_virtual_stars(node->nw, virtual_stars, current_num, max_virtual_stars);
        quad_tree_set_virtual_stars(node->sw, virtual_stars, current_num, max_virtual_stars);
        quad_tree_set_virtual_stars(node->se, virtual_stars, current_num, max_virtual_stars);
    }
    else if (*current_num < max_virtual_stars)
    {
        // TODO: handle mass more intelligently
        float x_sum = 0;
        float y_sum = 0;
        float mass_sum = 0;

        for (int i = 0; i < node->cell->num_stars; ++i)
        {
            x_sum += node->cell->stars[i]->x;
            y_sum += node->cell->stars[i]->y;
            mass_sum += node->cell->stars[i]->mass;
        }

        float x_avg = x_sum / node->cell->num_stars;
        float y_avg = y_sum / node->cell->num_stars;

        virtual_stars[*current_num].x = x_avg;
        virtual_stars[*current_num].y = y_avg;
        virtual_stars[*current_num].mass = mass_sum;
        (*current_num)++;
    }
    else
    {
        printf("Virtual_stars buffer overflow.\n");
    }
}


void quad_tree_stars_attract(struct QuadTreeNode *node, struct Star virtual_stars[], int num_virtual_stars)
{
    if (!node)
    {
        return;
    }

    if (!quad_tree_node_is_leaf(node))
    {
        quad_tree_stars_attract(node->ne, virtual_stars, num_virtual_stars);
        quad_tree_stars_attract(node->nw, virtual_stars, num_virtual_stars);
        quad_tree_stars_attract(node->sw, virtual_stars, num_virtual_stars);
        quad_tree_stars_attract(node->se, virtual_stars, num_virtual_stars);
    }
    else
    {
        for (int i = 0; i < node->cell->num_stars; ++i)
        {
            for (int j = i + 1; j < node->cell->num_stars; ++j)
            {
                star_attract(node->cell->stars[i], node->cell->stars[j]);

            }
            for (int k = 0; k < num_virtual_stars; ++k)
            {
                if (!cell_contains_star(node->cell, &virtual_stars[k]))
                {
                    star_attract(node->cell->stars[i], &virtual_stars[k]);
                }
            }
        }
    }
}


#endif
