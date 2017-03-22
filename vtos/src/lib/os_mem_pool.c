#include "os_mem_pool.h"
os_size_t os_mem_pool_create(void *addr, os_size_t mem_size, os_size_t blk_size)
{
	os_size_t ret = 1;
	if (blk_size > sizeof(void *) && mem_size > sizeof(struct mem_pool_def))
	{
		os_size_t nblks = (mem_size - sizeof(struct mem_pool_def)) / blk_size;
		if (nblks > 0)
		{
			void *paddr = (uint8 *)addr + sizeof(struct mem_pool_def);
			void **ppaddr = (void **)paddr;
			struct mem_pool_def *mem_pool = addr;
			os_size_t i;
			mem_pool->total_count = nblks;
			mem_pool->idle_count = nblks;
			mem_pool->start = paddr;
			mem_pool->block_size = blk_size;
			paddr = (uint8 *)paddr + blk_size;
			for (i = 0; i < nblks - 1; i++)
			{
				*ppaddr = paddr;
				ppaddr = (void **)paddr;
				paddr = (uint8 *)paddr + blk_size;
			}
			*ppaddr = NULL;
			ret = 0;
		}
	}
	return ret;
}

void *os_mem_block_get(void *addr)
{
	struct mem_pool_def *mem_pool = addr;
	void *ret = mem_pool->start;
	if (ret != NULL)
	{
		void **ppaddr = (void **)mem_pool->start;
		mem_pool->start = *ppaddr;
		mem_pool->idle_count--;
	}
	return ret;
}

void os_mem_block_put(void *addr, void *block)
{
	struct mem_pool_def *mem_pool = addr;
	os_size_t d = ((uint8 *)block - (uint8 *)addr - sizeof(struct mem_pool_def)) % mem_pool->block_size;
	if (0 == d)
	{
		void **ppaddr = (void **)block;
		if (NULL == mem_pool->start)
		{
			*ppaddr = NULL;
		}
		else
		{
			*ppaddr = mem_pool->start;
		}
		mem_pool->start = block;
		mem_pool->idle_count++;
	}
}

os_size_t os_total_block_count(void *addr)
{
	struct mem_pool_def *mem_pool = addr;
	return mem_pool->total_count;
}

os_size_t os_idle_block_count(void *addr)
{
	struct mem_pool_def *mem_pool = addr;
	return mem_pool->idle_count;
}

os_size_t os_block_size(void *addr)
{
	struct mem_pool_def *mem_pool = addr;
	return mem_pool->block_size;
}
