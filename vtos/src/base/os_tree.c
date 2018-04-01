#include "os_tree.h"
tree_node_type_def _leaf_node = 
{
	BLACK,       //color
	NULL,        //parent
	NULL,        //left_tree
	NULL,        //right_tree
};

static tree_node_type_def *grand_parent(tree_node_type_def *node)
{
	if (NULL == node->parent)
	{
		return NULL;
	}
	return node->parent->parent;
}

static tree_node_type_def *uncle(tree_node_type_def *node)
{
	if (NULL == grand_parent(node))
	{
		return NULL;
	}
	if (node->parent == grand_parent(node)->right_tree)
		return grand_parent(node)->left_tree;
	else
		return grand_parent(node)->right_tree;
}

static tree_node_type_def *sibling(tree_node_type_def *node)
{
	if (NULL == node->parent)
	{
		return NULL;
	}
	if (node == node->parent->left_tree)
		return node->parent->right_tree;
	else
		return node->parent->left_tree;
}

static void rotate_right(tree_node_type_def **handle, tree_node_type_def *node)
{
	tree_node_type_def *grand = grand_parent(node);
	tree_node_type_def *parent = node->parent;
	tree_node_type_def *right_tree = node->right_tree;

	parent->left_tree = right_tree;

	if (right_tree != &_leaf_node)
		right_tree->parent = parent;
	node->right_tree = parent;
	parent->parent = node;

	if (*handle == parent)
		*handle = node;
	node->parent = grand;

	if (grand != NULL)
	{
		if (grand->left_tree == parent)
			grand->left_tree = node;
		else
			grand->right_tree = node;
	}
}

static void rotate_left(tree_node_type_def **handle, tree_node_type_def *node)
{
	tree_node_type_def *grand = grand_parent(node);
	tree_node_type_def *parent = node->parent;
	tree_node_type_def *left_tree = node->left_tree;

	parent->right_tree = left_tree;

	if (left_tree != &_leaf_node)
		left_tree->parent = parent;
	node->left_tree = parent;
	parent->parent = node;

	if (*handle == parent)
		*handle = node;
	node->parent = grand;

	if (grand != NULL)
	{
		if (grand->left_tree == parent)
			grand->left_tree = node;
		else
			grand->right_tree = node;
	}
}

static void delete_case(tree_node_type_def **handle, tree_node_type_def *node)
{
	if (NULL == node->parent)
	{
		node->color = BLACK;
		return;
	}
	if (RED == sibling(node)->color)
	{
		node->parent->color = RED;
		sibling(node)->color = BLACK;
		if (node == node->parent->left_tree)
			rotate_left(handle, sibling(node));
		else
			rotate_right(handle, sibling(node));
	}
	if (BLACK == node->parent->color && BLACK == sibling(node)->color
			&& BLACK == sibling(node)->left_tree->color
			&& BLACK == sibling(node)->right_tree->color)
	{
		sibling(node)->color = RED;
		delete_case(handle, node->parent);
	}
	else if (RED == node->parent->color && BLACK == sibling(node)->color
			&& BLACK == sibling(node)->left_tree->color
			&& BLACK == sibling(node)->right_tree->color)
	{
		sibling(node)->color = RED;
		node->parent->color = BLACK;
	}
	else
	{
		if (BLACK == sibling(node)->color)
		{
			if (node == node->parent->left_tree
					&& RED == sibling(node)->left_tree->color
					&& BLACK == sibling(node)->right_tree->color)
			{
				sibling(node)->color = RED;
				sibling(node)->left_tree->color = BLACK;
				rotate_right(handle, sibling(node)->left_tree);
			}
			else if (node == node->parent->right_tree
					&& BLACK == sibling(node)->left_tree->color
					&& RED == sibling(node)->right_tree->color)
			{
				sibling(node)->color = RED;
				sibling(node)->right_tree->color = BLACK;
				rotate_left(handle, sibling(node)->right_tree);
			}
		}
		sibling(node)->color = node->parent->color;
		node->parent->color = BLACK;
		if (node == node->parent->left_tree)
		{
			sibling(node)->right_tree->color = BLACK;
			rotate_left(handle, sibling(node));
		}
		else
		{
			sibling(node)->left_tree->color = BLACK;
			rotate_right(handle, sibling(node));
		}
	}
}

void os_insert_case(tree_node_type_def **handle, tree_node_type_def *node)
{
	if (NULL == node->parent)
	{
		*handle = node;
		node->color = BLACK;
		return;
	}
	if (RED == node->parent->color)
	{
		if (RED == uncle(node)->color)
		{
			node->parent->color = uncle(node)->color = BLACK;
			grand_parent(node)->color = RED;
			os_insert_case(handle, grand_parent(node));
		}
		else
		{
			if (node->parent->right_tree == node
					&& grand_parent(node)->left_tree == node->parent)
			{
				rotate_left(handle, node);
				rotate_right(handle, node);
				node->color = BLACK;
				node->left_tree->color = node->right_tree->color = RED;
			}
			else if (node->parent->left_tree == node
					&& grand_parent(node)->right_tree == node->parent)
			{
				rotate_right(handle, node);
				rotate_left(handle ,node);
				node->color = BLACK;
				node->left_tree->color = node->right_tree->color = RED;
			}
			else if (node->parent->left_tree == node
					&& grand_parent(node)->left_tree == node->parent)
			{
				node->parent->color = BLACK;
				grand_parent(node)->color = RED;
				rotate_right(handle, node->parent);
			}
			else if (node->parent->right_tree == node
					&& grand_parent(node)->right_tree == node->parent)
			{
				node->parent->color = BLACK;
				grand_parent(node)->color = RED;
				rotate_left(handle, node->parent);
			}
		}
	}
}

void os_init_node(tree_node_type_def *node)
{
	node->color = RED;
	node->parent = NULL;
	node->left_tree = &_leaf_node;
	node->right_tree = &_leaf_node;
}

static void swap_zone(tree_node_type_def **handle, tree_node_type_def *node1, tree_node_type_def *node2)
{
	tree_node_type_def zone;
	zone.color = node2->color;
	zone.parent = node2->parent;
	zone.left_tree = node2->left_tree;
	zone.right_tree = node2->right_tree;
	if (node2 == node1->right_tree)
	{
		node2->color = node1->color;
		node2->parent = node1->parent;
		node2->left_tree = node1->left_tree;
		node2->right_tree = node1;
		if (node1->parent != NULL)
		{
			if (node1 == node1->parent->left_tree)
			{
				node1->parent->left_tree = node2;
			}
			else
			{
				node1->parent->right_tree = node2;
			}
		}
		else
		{
			*handle = node2;
		}
		node1->left_tree->parent = node2;

		node1->color = zone.color;
		node1->parent = node2;
		node1->left_tree = zone.left_tree;
		node1->right_tree = zone.right_tree;
		if (zone.left_tree != &_leaf_node)
		{
			zone.left_tree->parent = node1;
		}
		if (zone.right_tree != &_leaf_node)
		{
			zone.right_tree->parent = node1;
		}
	}
	else
	{
		node2->color = node1->color;
		node2->parent = node1->parent;
		node2->left_tree = node1->left_tree;
		node2->right_tree = node1->right_tree;
		if (node1->parent != NULL)
		{
			if (node1 == node1->parent->left_tree)
			{
				node1->parent->left_tree = node2;
			}
			else
			{
				node1->parent->right_tree = node2;
			}
		}
		else
		{
			*handle = node2;
		}
		if (node1->left_tree != &_leaf_node)
		{
			node1->left_tree->parent = node2;
		}
		if (node1->right_tree != &_leaf_node)
		{
			node1->right_tree->parent = node2;
		}
		node1->color = zone.color;
		node1->parent = zone.parent;
		node1->left_tree = zone.left_tree;
		node1->right_tree = zone.right_tree;
		zone.parent->left_tree = node1;
		if (zone.left_tree != &_leaf_node)
		{
			zone.left_tree->parent = node1;
		}
		if (zone.right_tree != &_leaf_node)
		{
			zone.right_tree->parent = node1;
		}
	}
}

static void do_delete_node(tree_node_type_def **handle, tree_node_type_def *node)
{
	tree_node_type_def *child =
			node->left_tree == &_leaf_node ? node->right_tree : node->left_tree;
	if (node->parent == NULL && node->left_tree == &_leaf_node
			&& node->right_tree == &_leaf_node)
	{
		*handle = NULL;
		return;
	}

	if (node->parent == NULL)
	{
		child->parent = NULL;
		*handle = child;
		(*handle)->color = BLACK;
		return;
	}

	if (node->parent->left_tree == node)
	{
		node->parent->left_tree = child;
	}
	else
	{
		node->parent->right_tree = child;
	}
	child->parent = node->parent;

	if (node->color == BLACK)
	{
		if (child->color == RED)
		{
			child->color = BLACK;
		}
		else
			delete_case(handle, child);
	}
}

void os_delete_node(tree_node_type_def **handle, tree_node_type_def *node)
{
	tree_node_type_def *curNode;
	if (node->right_tree == &_leaf_node)
	{
		do_delete_node(handle, node);
		return;
	}
	curNode = node->right_tree;
	while (curNode->left_tree != &_leaf_node)
	{
		curNode = curNode->left_tree;
	}
	swap_zone(handle, node, curNode);
	do_delete_node(handle, node);
}

tree_node_type_def *os_get_leftmost_node(tree_node_type_def **handle)
{
	tree_node_type_def *cur_node = NULL;
	if (*handle != NULL)
	{
		cur_node = *handle;
		while (cur_node->left_tree != &_leaf_node)
		{
			cur_node = cur_node->left_tree;
		}
	}
	return cur_node;
}

uint32 os_insert_node(tree_node_type_def **handle, tree_node_type_def *node, on_compare callback, void *arg)
{
	int32 cmp;
	tree_node_type_def *cur_node = *handle;
	os_init_node(node);
	if (NULL == *handle)
	{
		node->color = BLACK;
		*handle = node;
	}
	else
	{
		for (;;)
		{
			cmp = callback(node, cur_node, arg);
			if (cmp < 0)
			{
				if (cur_node->left_tree == &_leaf_node)
				{
					break;
				}
				cur_node = cur_node->left_tree;
			}
			else if(cmp > 0)
			{
				if (cur_node->right_tree == &_leaf_node)
				{
					break;
				}
				cur_node = cur_node->right_tree;
			}
			else
			{
				return 1;
			}
		}
		node->parent = cur_node;
		cmp = callback(node, cur_node, arg);
		if (cmp < 0)
			cur_node->left_tree = node;
		else if (cmp > 0)
			cur_node->right_tree = node;
		else
			return 1;
		os_insert_case(handle, node);
	}
	return 0;
}

tree_node_type_def *find_node(tree_node_type_def **handle, void *key, on_compare callback, void *arg)
{
	tree_node_type_def *cur = *handle;
	if (cur != NULL)
	{
		for (;;)
		{
			if (callback(key, cur, arg) == 0)
			{
				return cur;
			}
			else if (callback(key, cur, arg) < 0)
			{
				if (cur->left_tree == &_leaf_node)
				{
					break;
				}
				cur = cur->left_tree;
			}
			else
			{
				if (cur->right_tree == &_leaf_node)
				{
					break;
				}
				cur = cur->right_tree;
			}
		}
	}
	return NULL;
}
