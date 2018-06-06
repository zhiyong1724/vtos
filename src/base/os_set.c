#include "os_set.h"
#include "os_string.h"
#include "mem/os_mem.h"
#define LIST_NODE_ADDR_OFFSET sizeof(tree_node_type_def);
void os_set_init(os_set *obj, os_size_t unit_size)
{
	obj->tree = NULL;
	obj->head = NULL;
	obj->size = 0;
	obj->unit_size = unit_size;
}

void os_set_free(os_set *obj)
{
	os_set_iterator *itr = os_set_begin(obj);
	for (; itr != NULL; )
	{
		os_set_iterator *temp = itr;
		itr = os_set_next(obj, itr);
		os_free(temp);
	}
}

os_size_t os_set_size(os_set *obj)
{
	return obj->size;
}

os_size_t os_set_empty(os_set *obj)
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

static int8 compare(uint8 *arg1, uint8 *arg2, os_size_t size)
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

static int8 os_set_compare(void *key1, void *key2, void *arg)
{
	os_set_iterator *itr1 = (os_set_iterator *)key1;
	os_set_iterator *itr2 = (os_set_iterator *)key2;
	os_size_t *unit_size = arg;
	return compare((uint8 *)&itr1[1], (uint8 *)&itr2[1], *unit_size);
}

os_size_t os_set_insert(os_set *obj, void *data)
{
	os_set_iterator *itr = (os_set_iterator *)os_malloc(sizeof(os_set_iterator) + obj->unit_size);
	os_mem_cpy(&itr[1], data, obj->unit_size);
	if (os_insert_node(&obj->tree, &itr->tree_node, os_set_compare, &obj->unit_size) == 0)
	{
		os_insert_to_back(&obj->head, &itr->list_node);
		obj->size++;
		return 0;
	}
	os_free(itr);
	return 1;
}

static int8 os_set_find_compare(void *key1, void *key2, void *arg)
{
	os_set_iterator *itr = (os_set_iterator *)key2;
	os_size_t *unit_size = arg;
	return compare((uint8 *)key1, (uint8 *)&itr[1], *unit_size);
}

os_set_iterator *os_set_find(os_set *obj, void *data)
{
	if (obj->size > 0)
	{
		os_set_iterator *itr = (os_set_iterator *)find_node(obj->tree, data, os_set_find_compare, &obj->unit_size);
		return itr;
	}
	return NULL;
}

os_size_t os_set_erase(os_set *obj, os_set_iterator *itr)
{
	if (obj->size > 0)
	{
		os_delete_node(&obj->tree, &itr->tree_node);
		os_remove_from_list(&obj->head, &itr->list_node);
		os_free(itr);
		obj->size--;
		return 0;
	}
	return 1;
}

void os_set_clear(os_set *obj)
{
	os_set_iterator *itr = os_set_begin(obj);
	for (; itr != NULL; )
	{
		os_set_iterator *temp = itr;
		itr = os_set_next(obj, itr);
		os_free(temp);
	}
	obj->head = NULL;
	obj->tree = NULL;
	obj->size = 0;
}

void *os_set_value(os_set_iterator *itr)
{
	return &itr[1];
}

os_set_iterator *os_set_begin(os_set *obj)
{
	uint8 *ret = (uint8 *)obj->head;
	if (ret != NULL)
	{
		ret -= LIST_NODE_ADDR_OFFSET;
		return (os_set_iterator *)ret;
	}
	return NULL;
}

os_set_iterator *os_set_next(os_set *obj, os_set_iterator *itr)
{
	uint8 *ret = (uint8 *)itr->list_node.next_node;
	if (ret != (uint8 *)obj->head)
	{
		ret -= LIST_NODE_ADDR_OFFSET;
		return (os_set_iterator *)ret;
	}
	return NULL;
}