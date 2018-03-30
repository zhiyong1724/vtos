#ifndef __OS_TREE_H__
#define __OS_TREE_H__
#include "os_cpu.h"
#ifndef BLACK
#define BLACK 0
#endif

#ifndef RED
#define RED 1
#endif
#pragma pack()
typedef struct tree_node_type_def
{
	os_size_t color;
	struct tree_node_type_def *parent;
	struct tree_node_type_def *left_tree;
	struct tree_node_type_def *right_tree;
} tree_node_type_def;
#pragma pack()

typedef int32 (*on_compare)(tree_node_type_def *node1, tree_node_type_def *node2);

extern tree_node_type_def _leaf_node;

void os_init_node(tree_node_type_def *node);
void os_insert_case(tree_node_type_def **handle, tree_node_type_def *node);
/*********************************************************************************************************************
* 插入新节点
* handle 树根
* node：新节点
* callback：条件判断函数，小于0，新节点插入到左子树，大于0，新节点插入到右子树
* return：0：插入成功
*********************************************************************************************************************/
uint32 os_insert_node(tree_node_type_def **handle, tree_node_type_def *node, on_compare callback);
/*********************************************************************************************************************
* 删除节点
* handle 树根
* node：要删除的节点
*********************************************************************************************************************/
void os_delete_node(tree_node_type_def **handle, tree_node_type_def *node);
/*********************************************************************************************************************
* 获得最左边的叶子节点
* handle 树根
*********************************************************************************************************************/
tree_node_type_def *os_get_leftmost_node(tree_node_type_def **handle);
#endif
