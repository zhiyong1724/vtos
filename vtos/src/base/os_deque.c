#include "base/os_deque.h"
#include "base/os_string.h"
#include "mem/os_mem.h"
#define MAX_ARRAY_SIZE 8
void os_deque_init(os_deque *obj, os_size_t unit_size)
{
	obj->array = NULL;
	obj->unit_size = unit_size;
	obj->size = 0;
	obj->begin_index = 0;
	obj->end_index = 0;
}

void os_deque_free(os_deque *obj)
{
	list_node_type_def *cur = obj->array;
	if (cur != NULL)
	{
		list_node_type_def *origin = cur;
		for (; cur->next_node != origin; )
		{
			cur = cur->next_node;
			os_kfree(cur->pre_node);
		}
		os_kfree(cur);
	}
}

os_size_t os_deque_size(os_deque *obj)
{
	return obj->size;
}


os_size_t os_deque_empty(os_deque *obj)
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

static list_node_type_def *create_array(os_deque *obj)
{
	list_node_type_def *array = (list_node_type_def *)os_kmalloc(sizeof(list_node_type_def) + MAX_ARRAY_SIZE * obj->unit_size);
	return array;
}

os_size_t os_deque_push_back(os_deque *obj, void *data)
{
	if (MAX_ARRAY_SIZE - 1 == obj->end_index || NULL == obj->array)
	{
		obj->end_index = 0;
		list_node_type_def *node = create_array(obj);
		os_insert_to_back(&obj->array, node);
	}
	else
	{
		obj->end_index++;
	}
	list_node_type_def *end_list = obj->array->pre_node;
	uint8 *array = (uint8 *)&end_list[1];
	os_mem_cpy(&array[obj->end_index * obj->unit_size], data, obj->unit_size);
	obj->size++;
	return obj->size;
}

os_size_t os_deque_push_front(os_deque *obj, void *data)
{
	if (0 == obj->begin_index || NULL == obj->array)
	{
		obj->begin_index = MAX_ARRAY_SIZE - 1;
		list_node_type_def *node = create_array(obj);
		os_insert_to_front(&obj->array, node);
	}
	else
	{
		obj->begin_index--;
	}
	list_node_type_def *begin_list = obj->array;
	uint8 *array = (uint8 *)&begin_list[1];
	os_mem_cpy(&array[obj->begin_index * obj->unit_size], data, obj->unit_size);
	obj->size++;
	return obj->size;
}

void *os_deque_back(os_deque *obj)
{
	return os_deque_at(obj, obj->size - 1);
}

void *os_deque_front(os_deque *obj)
{
	return os_deque_at(obj, 0);
}

void *os_deque_at(os_deque *obj, os_size_t n)
{
	if (obj->size > 0)
	{
		if (n < obj->size)
		{
			list_node_type_def *cur = obj->array;
			os_size_t i = n / MAX_ARRAY_SIZE;
			os_size_t j = n % MAX_ARRAY_SIZE;
			j += obj->begin_index;
			if (j >= MAX_ARRAY_SIZE)
			{
				i++;
				j -= MAX_ARRAY_SIZE;
			}
			if (n < obj->size / 2)
			{
				for (; i > 0; i--)
				{
					cur = cur->next_node;
				}
			}
			else
			{
				os_size_t k = obj->size / MAX_ARRAY_SIZE;
				if (obj->size % MAX_ARRAY_SIZE > 0)
				{
					k++;
				}
				for (; k > i; k--)
				{
					cur = cur->pre_node;
				}
			}
			uint8 *array = (uint8 *)&cur[1];
			return &array[j * obj->unit_size];
		}
	}
	return NULL;
}

void os_deque_clear(os_deque *obj)
{
	os_deque_free(obj);
	obj->size = 0;
}

os_size_t os_deque_pop_back(os_deque *obj)
{
	if (obj->size > 0)
	{
		if (0 == obj->end_index || (obj->array == obj->array->next_node && obj->begin_index == obj->end_index))
		{
			obj->end_index = MAX_ARRAY_SIZE - 1;
			list_node_type_def *remove_node = obj->array->pre_node;
			os_remove_from_list(&obj->array, remove_node);
			os_kfree(remove_node);
		}
		else
		{
			obj->end_index--;
		}
		obj->size--;
		return 0;
	}
	return 1;
}

os_size_t os_deque_pop_front(os_deque *obj)
{
	if (obj->size > 0)
	{
		if (MAX_ARRAY_SIZE - 1 == obj->begin_index || (obj->array == obj->array->next_node && obj->begin_index == obj->end_index))
		{
			obj->begin_index = 0;
			list_node_type_def *remove_node = obj->array;
			os_remove_from_list(&obj->array, remove_node);
			os_kfree(remove_node);
		}
		else
		{
			obj->begin_index++;
		}
		obj->size--;
		return 0;
	}
	return 1;
}