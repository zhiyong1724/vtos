#include "os_mem_pool.h"
#include "os_string.h"
#include <stdlib.h>

static void link(void *root, os_size_t blk_size, os_size_t nblks)
{
	os_size_t i;
	void **ppaddr = (void **)root;
	void *paddr = (uint8 *)root + blk_size;
	for (i = 1; i < nblks; i++)
	{
		*ppaddr = paddr;
		ppaddr = (void **)paddr;
		paddr = (uint8 *)paddr + blk_size;
	}
	*ppaddr = NULL;
}

os_mem_pool *os_mem_pool_create(os_size_t blk_size)
{
	os_mem_pool *ret = NULL;
	if (blk_size >= sizeof(void *))
	{
		os_size_t nblks = (PAGE_SIZE - sizeof(os_mem_pool)) / blk_size;
		if (nblks >= 2)
		{
			ret = (os_mem_pool *)malloc(PAGE_SIZE);
			os_vector_init(&ret->segments, sizeof(void *));
			os_vector_push_back(&ret->segments, &ret);
			ret->total_count = nblks;
			ret->idle_count = nblks;
			ret->head = &ret[1];
			ret->block_size = blk_size;
			link(ret->head, ret->block_size, ret->total_count);
			return ret;
		}
		
	}
	return ret;
}

uint32 os_mem_pool_free(os_mem_pool *mem_pool)
{
	if (mem_pool->idle_count == mem_pool->total_count)
	{
		os_vector segments;
		os_size_t i;
		os_mem_cpy(&segments, &mem_pool->segments, sizeof(os_vector));
		for (i = 0; i < os_vector_size(&segments); i++)
		{
			void **p = os_vector_at(&segments, i);
			free(*p);
		}
		os_vector_free(&segments);
		return 0;
	}
	return 1;
}

static void os_mem_pool_expand(os_mem_pool *mem_pool)
{
	os_size_t nblks = PAGE_SIZE / mem_pool->block_size;
	void *new_segment = malloc(PAGE_SIZE);
	link(new_segment, mem_pool->block_size, nblks);
	os_vector_push_back(&mem_pool->segments, &new_segment);
	mem_pool->head = new_segment;
	mem_pool->idle_count += nblks;
	mem_pool->total_count += nblks;
}

void *os_mem_block_get(os_mem_pool *mem_pool)
{
	void **ppaddr;
	void *ret;
	if (NULL == mem_pool->head)
	{
		os_mem_pool_expand(mem_pool);
	}
	ret = mem_pool->head;
	ppaddr = (void **)mem_pool->head;
	mem_pool->head = *ppaddr;
	mem_pool->idle_count--;
	return ret;
}

void os_mem_block_put(os_mem_pool *mem_pool, void *block)
{
	if (mem_pool->idle_count < mem_pool->total_count)
	{
		void **ppaddr = (void **)block;
		if (NULL == mem_pool->head)
		{
			*ppaddr = NULL;
		}
		else
		{
			*ppaddr = mem_pool->head;
		}
		mem_pool->head = block;
		mem_pool->idle_count++;
	}
}

os_size_t os_total_block_count(os_mem_pool *mem_pool)
{
	return mem_pool->total_count;
}

os_size_t os_idle_block_count(os_mem_pool *mem_pool)
{
	return mem_pool->idle_count;
}

os_size_t os_block_size(os_mem_pool *mem_pool)
{
	return mem_pool->block_size;
}
