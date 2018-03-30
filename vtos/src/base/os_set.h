#ifndef __OS_SET_H__
#define __OS_SET_H__
#include "os_list.h"
#include "os_tree.h"
typedef struct os_set
{
	tree_node_type_def *tree;
	os_size_t unit_size;
	os_size_t size;
} os_set;
#endif