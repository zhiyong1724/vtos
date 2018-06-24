#ifndef __OS_MEM_POOL_H__
#define __OS_MEM_POOL_H__
#include "os_cpu.h"
typedef struct os_mem_pool
{
	void *head;
	os_size_t total_count;
	os_size_t idle_count;
	os_size_t block_size;
} os_mem_pool;
/*********************************************************************************************************************
* 初始化一个内存池
* pool：要初始化的内存池
* mem：连续块空间
* mem_size：内存大小
* blk_size：内存池中块的大小,这个块的大小必须要保证能放下一个地址
* return NULL：初始化失败
*********************************************************************************************************************/
os_mem_pool *os_mem_pool_init(os_mem_pool *pool, void *mem, os_size_t mem_size, os_size_t blk_size);
/*********************************************************************************************************************
* 从内存池中获取一个内存块
* mem_pool：指向被格式化的内存池
* return：返回块的地址，如果获取失败则返回NULL
*********************************************************************************************************************/
void *os_mem_block_get(os_mem_pool *mem_pool);
/*********************************************************************************************************************
* 释放一个内存块返回内存池，这个函数并没有安全检查功能，输入一个不相关的地址会出现不可预估的错误
* mem_pool：指向被格式化的内存池
* block：属于该内存池分配的内存块
*********************************************************************************************************************/
void os_mem_block_put(os_mem_pool *mem_pool, void *block);
/*********************************************************************************************************************
* 获取该内存池中块的数量
* os_mem_pool：指向被格式化的内存池
* return：返回该内存池中块的数量
*********************************************************************************************************************/
os_size_t os_total_block_count(os_mem_pool *mem_pool);
/*********************************************************************************************************************
* 获取该内存池中空闲块的数量
* os_mem_pool：指向被格式化的内存池
* return：返回该内存池中空闲块的数量
*********************************************************************************************************************/
os_size_t os_idle_block_count(os_mem_pool *mem_pool);
/*********************************************************************************************************************
* 获取该内存池块的大小
* os_mem_pool：指向被格式化的内存池
* return：返回该内存池块的大小
*********************************************************************************************************************/
os_size_t os_block_size(os_mem_pool *mem_pool);
#endif