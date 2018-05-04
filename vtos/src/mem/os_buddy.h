#ifndef __OS_BUDDY_H__
#define __OS_BUDDY_H__
#include "os_cpu.h"
#define GROUP_COUNT           21
struct buddy_block
{
	struct buddy_block *pre_block;
	struct buddy_block *next_block;
};

struct os_buddy
{
	uint8 *block_group;            //记录块属于哪一组
	uint8 *block_bitmap_array[GROUP_COUNT - 1];     //记录的块是否可合并
	struct buddy_block *index_array[GROUP_COUNT];     //记录可用块
	os_size_t size_array[GROUP_COUNT];               //记录每一组块的大小
	os_size_t max_group;
};
os_size_t os_buddy_init(void);
void *os_buddy_alloc(os_size_t size);
void os_buddy_free(void *addr);
#endif
