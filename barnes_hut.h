/* Quadtree implementation to keep track of recursive subdivisions of the
 * simulation field into rectangular cells.
 */

#ifndef BARNES_HUT_H
#define BARNES_HUT_H

// TODO: Why is there a segfault when this is set to 0?
// TODO: Why is there a segfault only very rarely when this is set to 1?
#define MAX_STARS_PER_CELL 1

struct Cell
{
    float center_x;
    float center_y;
    float distance_x;
    float distance_y;
};

struct QuadTreeNode
{
    struct Cell *cell;
    struct QuadTreeNode *children[4];
};


struct Cell* cell_init(float center_x, float center_y, float distance_x, float distance_y)
{
    struct Cell *cell = (struct Cell*)malloc(sizeof(struct Cell));
    if (cell)
    {
        cell->center_x = center_x;
        cell->center_y = center_y;
        cell->distance_x = distance_x;
        cell->distance_y = distance_y;
    }
    return cell;
}

int cell_count_stars_contained(struct Cell *c, struct Star stars[], int num_stars)
{
    int count = 0;
    for (int i = 0; i < num_stars; ++i)
    {
        if (stars[i].x <= c->center_x + c->distance_x
            && stars[i].x >= c->center_x - c->distance_x
            && stars[i].y <= c->center_y + c->distance_y
            && stars[i].y >= c->center_y - c->distance_y)
        {
            count++;
        }
    }
    return count;
}

struct QuadTreeNode* quad_tree_node_init(float center_x, float center_y, float distance_x, float distance_y)
{
    struct QuadTreeNode *node = (struct QuadTreeNode*)malloc(sizeof(struct QuadTreeNode));
    if (node)
    {
        node->cell = cell_init(center_x, center_y, distance_x, distance_y);
        for (int i = 0; i < 4; ++i)
        {
            node->children[i] = 0;
        }
    }
    return node;
}

void quad_tree_node_free(struct QuadTreeNode *node)
{
    if (node)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (node->children[i])
            {
                quad_tree_node_free(node->children[i]);
            }
        }
        free(node->cell);
        free(node);
    }
}

void quad_tree_node_subdivide(struct QuadTreeNode *node, struct Star stars[], int num_stars)
{
    if (cell_count_stars_contained(node->cell, stars, num_stars) > MAX_STARS_PER_CELL)
    {
        float child_distance_x = node->cell->distance_x / 2;
        float child_distance_y = node->cell->distance_y / 2;

        node->children[0] = quad_tree_node_init(node->cell->center_x + child_distance_x, node->cell->center_y + child_distance_y, child_distance_x, child_distance_y);
        node->children[1] = quad_tree_node_init(node->cell->center_x - child_distance_x, node->cell->center_y + child_distance_y, child_distance_x, child_distance_y);
        node->children[2] = quad_tree_node_init(node->cell->center_x - child_distance_x, node->cell->center_y - child_distance_y, child_distance_x, child_distance_y);
        node->children[3] = quad_tree_node_init(node->cell->center_x + child_distance_x, node->cell->center_y - child_distance_y, child_distance_x, child_distance_y);

        for (int i = 0; i < 4; ++i)
        {
            if (node->children[i])
            {
                quad_tree_node_subdivide(node->children[i], stars, num_stars);
            }
        }
    }
}

#endif
