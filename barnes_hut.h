/* Quadtree implementation to keep track of recursive subdivisions of the
 * simulation field into rectangular cells.
 */

#ifndef BARNES_HUT_H
#define BARNES_HUT_H

#define MAX_STARS_PER_CELL 1
// TODO: Limit tree depth

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


#endif
