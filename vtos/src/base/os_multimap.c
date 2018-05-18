#include "os_multimap.h"
#include "os_string.h"
#include "mem/os_mem.h"
void os_multimap_init(os_multimap *obj, os_size_t key_size, os_size_t value_size)
{
	os_map_init(&obj->map, key_size, value_size);
}

void os_multimap_free(os_multimap *obj)
{
	os_map_free(&obj->map);
}

os_size_t os_multimap_size(os_multimap *obj)
{
	return os_map_size(&obj->map);
}

os_size_t os_multimap_empty(os_multimap *obj)
{
	return os_map_empty(&obj->map);
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

static int8 os_multimap_compare(void *key1, void *key2, void *arg)
{
	os_multimap_iterator *itr1 = (os_multimap_iterator *)key1;
	os_multimap_iterator *itr2 = (os_multimap_iterator *)key2;
	os_size_t *key_size = arg;
	int8 ret = compare((uint8 *)&itr1[1], (uint8 *)&itr2[1], *key_size);
	if (0 == ret)
	{
		ret = -1;
	}
	return ret;
}

os_size_t os_multimap_insert(os_multimap *obj, void *key, void *value)
{
	uint8 *index;
	os_multimap_iterator *itr = (os_multimap_iterator *)os_malloc(sizeof(os_multimap_iterator) + obj->map.key_size + obj->map.value_size);
	index = (uint8 *)&itr[1];
	os_mem_cpy(index, key, obj->map.key_size);
	index += obj->map.key_size;
	os_mem_cpy(index, value, obj->map.value_size);
	if (os_insert_node(&obj->map.tree, &itr->map_iterator.tree_node, os_multimap_compare, &obj->map.key_size) == 0)
	{
		os_insert_to_back(&obj->map.head, &itr->map_iterator.list_node);
		obj->map.size++;
		return 0;
	}
	os_free(itr);
	return 1;
}

os_multimap_iterator *os_multimap_find(os_multimap *obj, void *key)
{
	return (os_multimap_iterator *)os_map_find(&obj->map, key);
}

os_size_t os_multimap_erase(os_multimap *obj, os_multimap_iterator *itr)
{
	return os_map_erase(&obj->map, &itr->map_iterator);
}

void os_multimap_clear(os_multimap *obj)
{
	os_map_clear(&obj->map);
}

void *os_multimap_first(os_multimap_iterator *itr)
{
	return os_map_first(&itr->map_iterator);
}

void *os_multimap_second(os_multimap *obj, os_multimap_iterator *itr)
{
	return os_map_second(&obj->map, &itr->map_iterator);
}

os_multimap_iterator *os_multimap_begin(os_multimap *obj)
{
	return (os_multimap_iterator *)os_map_begin(&obj->map);
}

os_multimap_iterator *os_multimap_next(os_multimap *obj, os_multimap_iterator *itr)
{
	return (os_multimap_iterator *)os_map_next(&obj->map, &itr->map_iterator);
}