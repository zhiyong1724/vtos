#ifndef _OS_MEM_H
#define _OS_MEM_H
#include "os_cpu.h"
#define GROUP_COUNT           11
struct block_node
{
	struct block_node *pre_block;
	struct block_node *next_block;
};

struct mem_controler
{
	uint8 *block_group;            //记录块属于哪一组
	uint8 *block_bitmap_array[GROUP_COUNT - 1];     //记录的块是否可合并
	struct block_node *index_array[GROUP_COUNT];     //记录可用块
	os_size_t size_array[GROUP_COUNT];               //记录每一组块的大小
};
os_size_t os_mem_init(void);
#endif
