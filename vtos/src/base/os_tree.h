#ifndef _OS_TREE_H
#define _OS_TREE_H
#include "os_tree_def.h"
#ifndef BLACK
#define BLACK 0
#endif

#ifndef RED
#define RED 1
#endif

extern tree_node_type_def _leaf_node;

void os_init_node(tree_node_type_def *node);
void os_insert_case(tree_node_type_def **handle, tree_node_type_def *node);
void os_delete_node(tree_node_type_def **handle, tree_node_type_def *node);
tree_node_type_def *os_get_leftmost_node(tree_node_type_def **handle);
#endif
