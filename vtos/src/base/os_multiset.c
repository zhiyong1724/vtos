#include "os_multiset.h"
#include "os_string.h"
#include "mem/os_mem.h"
void os_multiset_init(os_multiset *obj, os_size_t unit_size)
{
	os_set_init(&obj->set, unit_size);
}

void os_multiset_free(os_multiset *obj)
{
	os_set_free(&obj->set);
}

os_size_t os_multiset_size(os_multiset *obj)
{
	return os_set_size(&obj->set);
}

os_size_t os_multiset_empty(os_multiset *obj)
{
	return os_set_empty(&obj->set);
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

static int8 os_multiset_compare(void *key1, void *key2, void *arg)
{
	os_multiset_iterator *itr1 = (os_multiset_iterator *)key1;
	os_multiset_iterator *itr2 = (os_multiset_iterator *)key2;
	os_size_t *unit_size = arg;
	int8 ret = compare((uint8 *)&itr1[1], (uint8 *)&itr2[1], *unit_size);
	if (0 == ret)
	{
		ret = -1;
	}
	return ret;
}

os_size_t os_multiset_insert(os_multiset *obj, void *data)
{
	os_multiset_iterator *itr = (os_multiset_iterator *)os_kmalloc(sizeof(os_multiset_iterator) + obj->set.unit_size);
	os_mem_cpy(&itr[1], data, obj->set.unit_size);
	if (os_insert_node(&obj->set.tree, &itr->set_iterator.tree_node, os_multiset_compare, &obj->set.unit_size) == 0)
	{
		os_insert_to_back(&obj->set.head, &itr->set_iterator.list_node);
		obj->set.size++;
		return 0;
	}
	os_kfree(itr);
	return 1;
}

os_multiset_iterator *os_multiset_find(os_multiset *obj, void *data)
{
	return (os_multiset_iterator *)os_set_find(&obj->set, data);
}

os_size_t os_multiset_erase(os_multiset *obj, os_multiset_iterator *itr)
{
	return os_set_erase(&obj->set, &itr->set_iterator);
}

void os_multiset_clear(os_multiset *obj)
{
	os_set_clear(&obj->set);
}

void *os_multiset_value(os_multiset_iterator *itr)
{
	return os_set_value(&itr->set_iterator);
}

os_multiset_iterator *os_multiset_begin(os_multiset *obj)
{
	return (os_multiset_iterator *)os_set_begin(&obj->set);
}

os_multiset_iterator *os_multiset_next(os_multiset *obj, os_multiset_iterator *itr)
{
	return (os_multiset_iterator *)os_set_next(&obj->set, &itr->set_iterator);
}