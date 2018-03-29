#include "base/os_vector.h"
#include "base/os_string.h"
#include <stdlib.h>
void os_vector_init(os_vector *obj, os_size_t unit_size)
{
	obj->unit_size = unit_size;
	obj->size = 0;
	obj->max_size = 8;
	obj->buff = (uint8 *)malloc(obj->max_size * obj->unit_size);
}

void os_vector_free(os_vector *obj)
{
	free(obj->buff);
}

os_size_t os_vector_size(os_vector *obj)
{
	return obj->size;
}

os_size_t os_vector_max_size(os_vector *obj)
{
	return obj->max_size;
}

os_size_t os_vector_empty(os_vector *obj)
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

os_size_t os_vector_resize(os_vector *obj, os_size_t size)
{
	uint8 *new_buff = malloc(size * obj->unit_size);
	obj->max_size = size;
	os_mem_cpy(new_buff, obj->buff, obj->unit_size * obj->size);
	free(obj->buff);
	obj->buff = new_buff;
	return size;
}

os_size_t os_vector_push_back(os_vector *obj, void *data)
{
	return os_vector_insert(obj, data, obj->size);
}

os_size_t os_vector_push_front(os_vector *obj, void *data)
{
	return os_vector_insert(obj, data, 0);
}

os_size_t os_vector_insert(os_vector *obj, void *data, os_size_t n)
{
	os_size_t i;
	if (obj->size == obj->max_size)
	{
		os_vector_resize(obj, obj->max_size * 2);
	}
	for (i = obj->size; i > n; i--)
	{
		os_mem_cpy(&obj->buff[i * obj->unit_size], &obj->buff[(i - 1) * obj->unit_size], obj->unit_size);
	}
	os_mem_cpy(&obj->buff[n * obj->unit_size], data, obj->unit_size);
	obj->size++;
	return obj->size;
}

void *os_vector_back(os_vector *obj)
{
	if (obj->size > 0)
	{
		return os_vector_at(obj, obj->size - 1);
	}
	return NULL;
}

void *os_vector_front(os_vector *obj)
{
	if (obj->size > 0)
	{
		return os_vector_at(obj, 0);
	}
	return NULL;
}

void *os_vector_at(os_vector *obj, os_size_t n)
{
	if (n < obj->size)
	{
		return &obj->buff[n * obj->unit_size];
	}
	return NULL;
}

os_size_t os_vector_erase(os_vector *obj, os_size_t n)
{
	if (n < obj->size)
	{
		os_size_t i;
		for (i = n + 1; i < obj->size; i++)
		{
			os_mem_cpy(&obj->buff[(i - 1) * obj->unit_size], &obj->buff[i * obj->unit_size], obj->unit_size);
		}
		obj->size--;
	}
	return 1;
}

void os_vector_clear(os_vector *obj)
{
	obj->size = 0;
}

os_size_t os_vector_pop_back(os_vector *obj)
{
	return os_vector_erase(obj, obj->size - 1);
}

os_size_t os_vector_pop_front(os_vector *obj)
{
	return os_vector_erase(obj, 0);
}