#include "os_map.h"
#include "os_string.h"
#include <stdlib.h>
static const os_size_t LIST_NODE_ADDR_OFFSET = (os_size_t)sizeof(tree_node_type_def);
void os_map_init(os_map *obj, os_size_t key_size, os_size_t value_size)
{
	obj->tree = NULL;
	obj->head = NULL;
	obj->size = 0;
	obj->key_size = key_size;
	obj->value_size = value_size;
}

void os_map_free(os_map *obj)
{
	os_map_iterator *itr = os_map_begin(obj);
	for (; itr != NULL; )
	{
		os_map_iterator *temp = itr;
		itr = os_map_next(obj, itr);
		free(temp);
	}
}

os_size_t os_map_size(os_map *obj)
{
	return obj->size;
}

os_size_t os_map_empty(os_map *obj)
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

static os_size_t compare(uint8 *arg1, uint8 *arg2, os_size_t size)
{
	os_size_t i;
	for (i = 0; i < size; i++)
	{
		if (arg1[i] > arg2[i])
		{
			return 1;
		}
		else if (arg1[i] < arg2[i])
		{
			return -1;
		}
	}
	return 0;
}

static os_size_t os_map_compare(void *key1, void *key2, void *arg)
{
	os_map_iterator *itr1 = (os_map_iterator *)key1;
	os_map_iterator *itr2 = (os_map_iterator *)key2;
	os_size_t *key_size = arg;
	return compare((uint8 *)&itr1[1], (uint8 *)&itr2[1], *key_size);
}

os_size_t os_map_insert(os_map *obj, void *key, void *value)
{
	uint8 *index;
	os_map_iterator *itr = (os_map_iterator *)malloc(sizeof(os_map_iterator) + obj->key_size + obj->value_size);
	index = (uint8 *)&itr[1];
	os_mem_cpy(index, key, obj->key_size);
	index += obj->key_size;
	os_mem_cpy(index, value, obj->value_size);
	if (os_insert_node(&obj->tree, &itr->tree_node, os_map_compare, &obj->key_size) == 0)
	{
		os_insert_to_back(&obj->head, &itr->list_node);
		obj->size++;
		return 0;
	}
	free(itr);
	return 1;
}

static os_size_t os_map_find_compare(void *key1, void *key2, void *arg)
{
	os_map_iterator *itr = (os_map_iterator *)key2;
	os_size_t *key_size = arg;
	return compare((uint8 *)key1, (uint8 *)&itr[1], *key_size);
}

os_map_iterator *os_map_find(os_map *obj, void *key)
{
	if (obj->size > 0)
	{
		os_map_iterator *itr = (os_map_iterator *)find_node(obj->tree, key, os_map_find_compare, &obj->key_size);
		return itr;
	}
	return NULL;
}

os_size_t os_map_erase(os_map *obj, os_map_iterator *itr)
{
	if (obj->size > 0)
	{
		os_delete_node(&obj->tree, &itr->tree_node);
		os_remove_from_list(&obj->head, &itr->list_node);
		free(itr);
		obj->size--;
		return 0;
	}
	return 1;
}

void os_map_clear(os_map *obj)
{
	os_map_iterator *itr = os_map_begin(obj);
	for (; itr != NULL; )
	{
		os_map_iterator *temp = itr;
		itr = os_map_next(obj, itr);
		free(temp);
	}
	obj->head = NULL;
	obj->tree = NULL;
	obj->size = 0;
}

void *os_map_first(os_map_iterator *itr)
{
	return &itr[1];
}

void *os_map_second(os_map *obj, os_map_iterator *itr)
{
	uint8 *index = (uint8 *)&itr[1];
	index += obj->key_size;
	return index;
}

os_map_iterator *os_map_begin(os_map *obj)
{
	uint8 *ret = (uint8 *)obj->head;
	if (ret != NULL)
	{
		ret -= LIST_NODE_ADDR_OFFSET;
		return (os_map_iterator *)ret;
	}
	return NULL;
}

os_map_iterator *os_map_next(os_map *obj, os_map_iterator *itr)
{
	uint8 *ret = (uint8 *)itr->list_node.next_node;
	if (ret != (uint8 *)obj->head)
	{
		ret -= LIST_NODE_ADDR_OFFSET;
		return (os_map_iterator *)ret;
	}
	return NULL;
}