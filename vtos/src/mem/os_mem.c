#include "os_mem.h"
#include "vtos.h"
#include "os_buddy.h"
#include "base/os_mem_pool.h"
#ifdef __USE_STD_MALLOC__
#include <stdlib.h>
#endif
static struct os_mem _os_mem;

static os_size_t get_std_size(os_size_t size, os_size_t min_size)
{
	os_size_t ret = min_size;
	for (; size > ret;)
	{
		ret *= 2;
	}
	return ret;
}

static mem_pool_node *create_mem_pool(os_size_t blk_size)
{
	mem_pool_node *pool = (mem_pool_node *)os_buddy_alloc(PAGE_SIZE);
	if (pool != NULL)
	{
		os_mem_pool_init(&pool->pool, &pool[1], PAGE_SIZE - sizeof(mem_pool_node), blk_size);
		pool->magic = 0xaa55;
		_os_mem.free_size -= sizeof(mem_pool_node);
	}
	return pool;
}

static os_size_t on_insert_compare(void *key1, void *key2, void *arg)
{
	mem_pool_node *pool1 = (mem_pool_node *)key1;
	mem_pool_node *pool2 = (mem_pool_node *)key2;
	if (pool1->pool.block_size <= pool2->pool.block_size)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

static os_size_t on_find_compare(void *key1, void *key2, void *arg)
{
	os_size_t *std_size = (os_size_t *)key1;
	mem_pool_node *pool = (mem_pool_node *)key2;
	if (*std_size < pool->pool.block_size)
	{
		return -1;
	}
	else if (*std_size > pool->pool.block_size)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

os_size_t os_mem_init(void)
{
	_os_mem.root = NULL;
	_os_mem.total_size = os_buddy_init();
	_os_mem.free_size = _os_mem.total_size;
	if (_os_mem.total_size > 0)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

void *os_kmalloc(os_size_t size)
{
#ifdef __USE_STD_MALLOC__
	return malloc(size);
#else
	uint8 *addr = NULL;
	os_size_t std_size = get_std_size(size + sizeof(void *), MIN_BLOCK_SIZE);
	if (std_size < PAGE_SIZE / 2)
	{
		//使用高速内存
		for (; std_size < PAGE_SIZE / 2; std_size *= 2)
		{
			mem_pool_node *pool = (mem_pool_node *)find_node(_os_mem.root, &std_size, on_find_compare, NULL);
			if (NULL == pool)
			{
				pool = create_mem_pool(std_size);
				if (pool != NULL)
				{
					os_insert_node(&_os_mem.root, &pool->node, on_insert_compare, NULL);
				}
			}
			if (pool != NULL)
			{
				addr = os_mem_block_get(&pool->pool);
				*((void **)addr) = pool;
				addr += sizeof(void *);
				if (0 == os_idle_block_count(&pool->pool))
				{
					os_delete_node(&_os_mem.root, &pool->node);
				}
				_os_mem.free_size -= std_size;
				break;
			}
		}
	}
	else
	{
		//使用伙伴算法
		std_size = get_std_size(size, PAGE_SIZE);
		addr = os_buddy_alloc(std_size);
		if (addr != NULL)
		{
			_os_mem.free_size -= std_size;
		}
	}
	return addr;
#endif
}

void os_kfree(void *addr)
{
#ifdef __USE_STD_MALLOC__
	free(addr);
#else
	os_size_t size = os_buddy_free(addr);
	if (0 == size)
	{
		//属于高速内存
		mem_pool_node *pool = *((mem_pool_node **)((uint8 *)addr - sizeof(void *)));
		if (0xaa55 == pool->magic)
		{
			os_mem_block_put(&pool->pool, addr);
			_os_mem.free_size += pool->pool.block_size;
			if (1 == os_idle_block_count(&pool->pool))
			{
				os_insert_node(&_os_mem.root, &pool->node, on_insert_compare, NULL);
			}
			else if (os_idle_block_count(&pool->pool) == os_total_block_count(&pool->pool))
			{
				os_buddy_free(pool);
				_os_mem.free_size += sizeof(mem_pool_node);
			}
		}
	}
	else
	{
		//属于伙伴算法
		_os_mem.free_size += size;
	}
#endif
}

os_size_t os_get_total_size()
{
	return _os_mem.total_size;
}

os_size_t os_get_free_size()
{
	return _os_mem.free_size;
}
