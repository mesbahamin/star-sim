/* Quadtree implementation to keep track of recursive subdivisions of the
 * simulation field into rectangular cells.
 */

#ifndef BARNES_HUT_H
#define BARNES_HUT_H

#include <stdbool.h>
// TODO: Remove this.
#include "star.h"

// TODO: Limit tree depth
#ifndef THETA
#define THETA 0.5f
#endif

struct Cell
{
    float center_x;
    float center_y;
    float distance_x;
    float distance_y;
    float mass_total;
    float mass_center_x;
    float mass_center_y;
    float stars_sum_x;
    float stars_sum_y;
    // TODO: Make this a void*.
    struct Star *star;
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


struct Cell *cell_init(float center_x, float center_y, float distance_x, float distance_y);
void cell_free(struct Cell *c);
bool cell_contains_star(struct Cell *c, struct Star *s);
bool cell_is_empty(struct Cell *c);

struct QuadTreeNode *quad_tree_node_init(float center_x, float center_y, float distance_x, float distance_y);
void quad_tree_node_free(struct QuadTreeNode *node);
bool quad_tree_node_is_leaf(struct QuadTreeNode *node);
void quad_tree_node_subdivide(struct QuadTreeNode *node);
void quad_tree_node_insert_star(struct QuadTreeNode *node, struct Star *star);

struct QuadTree *quad_tree_init(void);
void quad_tree_free(struct QuadTree *qt);
bool node_is_sufficiently_far(struct QuadTreeNode *node, struct Star *star);
void quad_tree_calc_force_on_star(struct QuadTreeNode *node, struct Star *star);

#endif
