#include "os_mem_pool.h"
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

os_mem_pool *os_mem_pool_init(os_mem_pool *pool, void *mem, os_size_t mem_size, os_size_t blk_size)
{
	os_mem_pool *ret = NULL;
	if (blk_size >= sizeof(void *))
	{
		os_size_t nblks = mem_size / blk_size;
		if (nblks >= 2)
		{
			ret = pool;
			ret->total_count = nblks;
			ret->idle_count = nblks;
			ret->head = mem;
			ret->block_size = blk_size;
			link(ret->head, ret->block_size, ret->total_count);
			return ret;
		}
		
	}
	return ret;
}

void *os_mem_block_get(os_mem_pool *mem_pool)
{
	void **ppaddr;
	void *ret = NULL;
	if (mem_pool->idle_count > 0)
	{
		ret = mem_pool->head;
		ppaddr = (void **)mem_pool->head;
		mem_pool->head = *ppaddr;
		mem_pool->idle_count--;
	}
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
