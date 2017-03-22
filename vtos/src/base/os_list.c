#include "os_list.h"
void os_insert_to_front(list_node_type_def **handle, list_node_type_def *node)
{
	if (NULL == *handle)
	{
		node->pre_node = node;
		node->next_node = node;
	}
	else
	{
		node->pre_node = (*handle)->pre_node;
		node->next_node = *handle;
		(*handle)->pre_node = node;
		node->pre_node->next_node = node;
	}
	*handle = node;
}

void os_insert_to_back(list_node_type_def **handle, list_node_type_def *node)
{
	if (NULL == *handle)
	{
		node->pre_node = node;
		node->next_node = node;
		*handle = node;
	}
	else
	{
		node->pre_node = (*handle)->pre_node;
		node->next_node = *handle;
		(*handle)->pre_node = node;
		node->pre_node->next_node = node;
	}
	*handle = node->pre_node;
}

void os_remove_from_list(list_node_type_def **handle, list_node_type_def *node)
{
	if (node->pre_node == node
		&& node->next_node == node)
	{
		*handle = NULL;
	}
	else
	{
		node->pre_node->next_node =
			node->next_node;
		node->next_node->pre_node =
			node->pre_node;
		if (*handle == node)
		{
			*handle = node->next_node;
		}
	}
}

list_node_type_def *os_get_back_from_list(list_node_type_def **handle)
{
	return (*handle)->pre_node;
}
