#ifndef __OS_TREE_DEF_H__
#define __OS_TREE_DEF_H__
#include "os_cpu.h"
typedef struct tree_node_type_def
{
	os_size_t color;
	struct tree_node_type_def *parent;
	struct tree_node_type_def *left_tree;
	struct tree_node_type_def *right_tree;
} tree_node_type_def;
#endif