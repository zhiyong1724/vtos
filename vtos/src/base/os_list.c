#include "os_list.h"
#include "os_string.h"
#include <stdlib.h>
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
	if (*handle != NULL)
	{
		return (*handle)->pre_node;
	}
	return NULL;
}

void os_list_init(os_list *obj, os_size_t unit_size)
{
	obj->list = NULL;
	obj->unit_size = unit_size;
	obj->size = 0;
}

void os_list_free(os_list *obj)
{
	list_node_type_def *node = os_get_back_from_list(&obj->list);
	for (; node != NULL; node = os_get_back_from_list(&obj->list))
	{
		os_remove_from_list(&obj->list, node);
		free(node);
	}
}

os_size_t os_list_size(os_list *obj)
{
	return obj->size;
}

os_size_t os_list_empty(os_list *obj)
{
	if (obj->size > 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

os_size_t os_list_push_back(os_list *obj, void *data)
{
	list_node_type_def *node = (list_node_type_def *)malloc(sizeof(list_node_type_def) + obj->unit_size);
	os_mem_cpy(&node[1], data, obj->unit_size);
	os_insert_to_back(&obj->list, node);
	obj->size++;
	return obj->size;
}

os_size_t os_list_push_front(os_list *obj, void *data)
{
	list_node_type_def *node = (list_node_type_def *)malloc(sizeof(list_node_type_def) + obj->unit_size);
	os_mem_cpy(&node[1], data, obj->unit_size);
	os_insert_to_front(&obj->list, node);
	obj->size++;
	return obj->size;
}

os_size_t os_list_insert(os_list *obj, void *data, os_size_t n)
{
	if (0 == n)
	{
		return os_list_push_front(obj, data);
	}
	else if (n < obj->size)
	{
		list_node_type_def *node;
		list_node_type_def *cur = obj->list;
		os_size_t i;
		if (n <= obj->size / 2)
		{
			for (i = 0; i < n; i++)
			{
				cur = cur->next_node;
			}
		}
		else
		{
			for (i = n; i < obj->size; i++)
			{
				cur = cur->pre_node;
			}
		}
		node = (list_node_type_def *)malloc(sizeof(list_node_type_def) + obj->unit_size);
		os_mem_cpy(&node[1], data, obj->unit_size);
		os_insert_to_front(&cur, node);
	}
	else if (n == obj->size)
	{
		return os_list_push_back(obj, data);
	}
	obj->size++;
	return obj->size;
}

void *os_list_back(os_list *obj)
{
	if (obj->size > 0)
	{
		list_node_type_def *ret = obj->list->pre_node;
		return &ret[1];
	}
	return NULL;
}

void *os_list_front(os_list *obj)
{
	if (obj->size > 0)
	{
		list_node_type_def *ret = obj->list;
		return &ret[1];
	}
	return NULL;
}

void *os_list_at(os_list *obj, os_size_t n)
{
	if (obj->size > 0)
	{
		if (0 == n)
		{
			return os_list_front(obj);
		}
		else if (n < obj->size)
		{
			list_node_type_def *cur = obj->list;
			os_size_t i;
			if (n <= obj->size / 2)
			{
				for (i = 0; i < n; i++)
				{
					cur = cur->next_node;
				}
			}
			else
			{
				for (i = n; i < obj->size; i++)
				{
					cur = cur->pre_node;
				}
			}
			return &cur[1];
		}
		else if (n == obj->size - 1)
		{
			return os_list_back(obj);
		}
	}
	return NULL;
}

os_size_t os_list_erase(os_list *obj, os_size_t n)
{
	if (n < obj->size)
	{
		if (0 == n)
		{
			return os_list_pop_front(obj);
		}
		else if (n == obj->size - 1)
		{
			return os_list_pop_back(obj);
		}
		else
		{
			list_node_type_def *cur = obj->list;
			os_size_t i;
			if (n <= obj->size / 2)
			{
				for (i = 0; i < n; i++)
				{
					cur = cur->next_node;
				}
			}
			else
			{
				for (i = n; i < obj->size; i++)
				{
					cur = cur->pre_node;
				}
			}
			os_remove_from_list(&obj->list, cur);
			obj->size--;
			free(cur);
			return 0;
		}
	}
	return 1;
}

void os_list_clear(os_list *obj)
{
	os_list_free(obj);
	obj->size = 0;
}

os_size_t os_list_pop_back(os_list *obj)
{
	if (obj->size > 0)
	{
		list_node_type_def *node = obj->list->pre_node;
		os_remove_from_list(&obj->list, node);
		obj->size--;
		free(node);
		return 0;
	}
	return 1;
}

os_size_t os_list_pop_front(os_list *obj)
{
	if (obj->size > 0)
	{
		list_node_type_def *node = obj->list;
		os_remove_from_list(&obj->list, node);
		obj->size--;
		free(node);
		return 0;
	}
	return 1;
}

