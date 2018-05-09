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
	for (; cur != obj->array; cur = cur->next_node)
	{
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
	if (MAX_ARRAY_SIZE - 1 == obj->begin_index)
	{
		obj->end_index = 0;
		list_node_type_def *node = create_array(obj);
		os_insert_to_back(&obj->array, node);
	}
	list_node_type_def *end_list = obj->array->pre_node;
	uint8 *array = (uint8 *)&end_list[1];
	os_mem_cpy(&array[obj->begin_index], data, obj->unit_size);
	obj->begin_index++;
	return obj->size;
}

os_size_t os_deque_push_front(os_deque *obj, void *data)
{
	if (0 == obj->begin_index)
	{
		obj->begin_index = MAX_ARRAY_SIZE - 1;
		list_node_type_def *node = create_array(obj);
		os_insert_to_front(&obj->array, node);
	}
	uint8 *array = (uint8 *)&obj->array[1];
	os_mem_cpy(&array[obj->begin_index], data, obj->unit_size);
	obj->begin_index--;
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
			os_size_t i;
			list_node_type_def *cur = obj->array;
			if (n <= obj->size / 2)
			{
				os_size_t cur_size = MAX_ARRAY_SIZE - obj->begin_index;
				for (; n >= cur_size; cur_size += MAX_ARRAY_SIZE)
				{
					cur = cur->next_node;
				}
				if (n < MAX_ARRAY_SIZE - obj->begin_index)
				{
					i = MAX_ARRAY_SIZE - obj->begin_index + n;
				}
				else
				{
					i = n - (MAX_ARRAY_SIZE - obj->begin_index);
					i %= MAX_ARRAY_SIZE;
				}
			}
			else
			{
				cur = cur->pre_node;
				os_size_t cur_size = obj->end_index + 1;
				for (; obj->size - n > cur_size; cur_size += MAX_ARRAY_SIZE)
				{
					cur = cur->pre_node;
				}
				if (obj->size - n <= obj->end_index + 1)
				{
					i = obj->end_index;
				}
				else
				{
					i = MAX_ARRAY_SIZE - obj->begin_index - 1;
					i %= MAX_ARRAY_SIZE;
					i = MAX_ARRAY_SIZE - i - 1;
				}
			}
			uint8 *array = (uint8 *)&obj->array[1];
			return &array[i];
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
		if (MAX_ARRAY_SIZE - 1 == obj->end_index)
		{
			os_remove_from_list(&obj->array, obj->array);
		}
		else
		{
			obj->end_index++;
		}
		return 0;
	}
	return 1;
}

os_size_t os_deques_pop_front(os_deque *obj)
{
	if (obj->size > 0)
	{
		if (0 == obj->end_index)
		{
			os_remove_from_list(&obj->array, obj->array->pre_node);
		}
		else
		{
			obj->end_index--;
		}
		return 0;
	}
	return 1;
}