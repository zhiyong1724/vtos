#include "os_mem.h"
#include "base/os_string.h"
#include "os_buddy.h"
void *_start_addr = NULL;
void *_end_addr = NULL;
static struct os_buddy _os_buddy;
static os_size_t pow2x(os_size_t n)
{
	os_size_t ret = 1;
	for (; n > 0; n--)
	{
		ret *= 2;
	}
	return ret;
}

static void insert_mem_block(os_size_t group, struct buddy_block *new_block)
{
	new_block->pre_block = NULL;
	if (NULL == _os_buddy.index_array[group])
	{
		new_block->next_block = NULL;
	}
	else
	{
		new_block->next_block = _os_buddy.index_array[group];
		_os_buddy.index_array[group]->pre_block = new_block;
	}
	_os_buddy.index_array[group] = new_block;
}

static os_size_t format_mem(uint8 *start_addr, os_size_t group, os_size_t addr_space)
{
	os_size_t block_size = PAGE_SIZE * pow2x(group);
	if (addr_space >= block_size)
	{
		os_size_t count = addr_space / block_size;
		os_size_t i = 0;
		for (i = 0; i < count; i++)
		{
			insert_mem_block(group, (struct buddy_block *)start_addr);
			start_addr += block_size;
			addr_space -= block_size;
		}
		if (group < _os_buddy.max_group && count % 2 != 0)
		{
			os_size_t offset = 0;
			os_size_t x = 0;
			os_size_t y = 0;
			uint8 mark_bit = 0x80;
			offset = ((uint8 *)start_addr - (uint8 *)_start_addr - block_size)
				/ _os_buddy.size_array[group + 1];
			x = offset >> 3;
			y = offset % 8;
			mark_bit >>= y;
			_os_buddy.block_bitmap_array[group][x] ^= mark_bit;
		}
	}
	if (group > 0)
	{
		group--;
		addr_space = format_mem(start_addr, group, addr_space);
	}
	return addr_space;
}

static void do_buddy_init(uint8 *index, os_size_t page_count)
{
	os_size_t size = page_count;
	_os_buddy.block_group = index;
	os_mem_set(_os_buddy.block_group, GROUP_COUNT, size);  //初始化block_group
	index += size;
	{
		os_size_t i = 0;
		size = (((page_count >> 1) + 1) >> 3) + 1;
		for (i = 0; i < GROUP_COUNT - 1; i++)
		{
			//初始化block_bitmap
			if (i < _os_buddy.max_group)
			{
				_os_buddy.block_bitmap_array[i] = index;
				os_mem_set(_os_buddy.block_bitmap_array[i], 0, size);
				index += size;
				size = (size >> 1) + 1;
			}
			else
			{
				_os_buddy.block_bitmap_array[i] = NULL;
			}
		}
		for (i = 0; i < GROUP_COUNT; i++)
		{
			//初始化index_array
			_os_buddy.index_array[i] = NULL;
		}
		for (i = 0; i < GROUP_COUNT; i++)
		{
			//初始化size_array
			_os_buddy.size_array[i] = PAGE_SIZE * pow2x(i);
		}
	}
}

static os_size_t create_os_buddy(os_size_t group)
{
	uint8 *start_addr = (uint8 *)_start_addr;
	uint8 *end_addr = (uint8 *)_end_addr;
	os_size_t block_size = PAGE_SIZE * pow2x(group);
	os_size_t addr_space = (os_size_t)(end_addr - start_addr);
	if (block_size > 0 && block_size < addr_space)
	{
		os_size_t i = 0;
		os_size_t page_count = addr_space / PAGE_SIZE;
		os_size_t total_size = page_count; //block_group空间，每个字节记录一页
		os_size_t block_bitmap_size = (((page_count >> 1) + 1) >> 3) + 1;         //block_bitmap_0空间，加一是为了防止除不尽，空间留多一点无妨
		total_size += block_bitmap_size;
		for (i = 1; i < group; i++)
		{
			block_bitmap_size = (block_bitmap_size >> 1) + 1;
			total_size += block_bitmap_size;
		}
		if (addr_space > block_size + total_size)
		{
			//到这里表示地址空间够用，可以初始化这一组
			_os_buddy.max_group = group;
			addr_space -= total_size;
			do_buddy_init(end_addr - total_size, page_count);
			format_mem(start_addr, group, addr_space);
		}
	}
	else
	{
		addr_space = 0;
	}
	return addr_space;
}

os_size_t os_buddy_init(void)
{
	os_size_t size = 0;
	os_size_t i = GROUP_COUNT - 1;
	os_get_start_addr();
	if (_start_addr != NULL && _end_addr > _start_addr)
	{
		for (;; i--)
		{
			size = create_os_buddy(i);
			if (size > 0)
			{
				break;
			}
		}
	}
	return size;
}

static void delete_mem_block(os_size_t group, struct buddy_block *block)
{
	if (NULL == block->pre_block && NULL == block->next_block)
	{
		_os_buddy.index_array[group] = NULL;
	}
	else if (NULL == block->pre_block && block->next_block != NULL)
	{
		block->next_block->pre_block = NULL;
		_os_buddy.index_array[group] = block->next_block;
	}
	else if (block->pre_block != NULL && NULL == block->next_block)
	{
		block->pre_block->next_block = NULL;
	}
	else
	{
		block->pre_block->next_block = block->next_block;
		block->next_block->pre_block = block->pre_block;
	}
}

static void *get_block(os_size_t group)
{
	void *ret = NULL;
	while (1)
	{
		if (group <= _os_buddy.max_group)
		{
			if (_os_buddy.index_array[group] != NULL)
			{
				ret = _os_buddy.index_array[group];
				delete_mem_block(group, _os_buddy.index_array[group]);
				if (group < _os_buddy.max_group)
				{
					os_size_t offset = 0;
					os_size_t i = 0;
					os_size_t j = 0;
					uint8 mark_bit = 0x80;
					offset = ((uint8 *)ret - (uint8 *)_start_addr)
						/ _os_buddy.size_array[group + 1];
					i = offset >> 3;
					j = offset % 8;
					mark_bit >>= j;
					_os_buddy.block_bitmap_array[group][i] ^= mark_bit;
				}
			}
			else
			{
				ret = get_block(group + 1);
				if (ret != NULL)
				{
					void *next_addr = (uint8 *)ret + _os_buddy.size_array[group];
					insert_mem_block(group, (struct buddy_block *) next_addr);
					if (group < _os_buddy.max_group)
					{
						os_size_t offset = 0;
						os_size_t i = 0;
						os_size_t j = 0;
						uint8 mark_bit = 0x80;
						offset = ((uint8 *)ret - (uint8 *)_start_addr)
							/ _os_buddy.size_array[group + 1];
						i = offset >> 3;
						j = offset % 8;
						mark_bit >>= j;
						_os_buddy.block_bitmap_array[group][i] ^= mark_bit;
					}
				}
			}
		}
		if (NULL == ret)
		{
		}
		break;
	}
	return ret;
}

static void free_block(os_size_t group, void *addr)
{
	if (group < _os_buddy.max_group)
	{
		//不是最后一组
		os_size_t offset = ((uint8 *)addr - (uint8 *)_start_addr)
			/ _os_buddy.size_array[group + 1];
		os_size_t i = offset >> 3;
		os_size_t j = offset % 8;
		uint8 mark_bit = 0x80;
		mark_bit >>= j;
		mark_bit &= _os_buddy.block_bitmap_array[group][i];
		if (mark_bit > 0)
		{
			//表示可以合并
			if (0
				== ((uint8 *)addr - (uint8 *)_start_addr)
				% _os_buddy.size_array[group + 1])
			{
				//释放后面的伙伴
				delete_mem_block(group,
					(struct buddy_block *) ((uint8 *)addr
						+ _os_buddy.size_array[group]));
				free_block(group + 1, addr);
			}
			else
			{
				//释放前面的伙伴
				delete_mem_block(group,
					(struct buddy_block *) ((uint8 *)addr
						- _os_buddy.size_array[group]));
				free_block(group + 1, (uint8 *)addr - _os_buddy.size_array[group]);
			}
		}
		else
		{
			//不可以合并就直接插入链表中
			insert_mem_block(group, (struct buddy_block *) addr);
		}
		mark_bit = 0x80;
		mark_bit >>= j;
		_os_buddy.block_bitmap_array[group][i] ^= mark_bit;
	}
	else
	{
		//最后一组直接插入链表中
		insert_mem_block(group, (struct buddy_block *) addr);
	}
}

void *os_buddy_alloc(os_size_t size)
{
	void *ret = NULL;
	os_size_t i;
	for (i = 0; i < GROUP_COUNT - 1; i++)
	{
		if (size <= _os_buddy.size_array[i])
		{
			ret = get_block(i);
			if (ret != NULL)
			{
				os_size_t j = ((uint8 *)ret - (uint8 *)_start_addr) / _os_buddy.size_array[0];
				_os_buddy.block_group[j] = (uint8)i;
				break;
			}
		}
	}
	return ret;
}

os_size_t os_buddy_free(void *addr)
{
	os_size_t size = 0;
	if (addr >= (void *)_start_addr)
	{
		os_size_t addr_offset = (uint8 *)addr - (uint8 *)_start_addr;
		if (0 == addr_offset % _os_buddy.size_array[0])
		{
			//有效的地址
			os_size_t i = addr_offset / _os_buddy.size_array[0];
			if (_os_buddy.block_group[i] != GROUP_COUNT)
			{
				free_block(_os_buddy.block_group[i], addr);
				size = _os_buddy.size_array[_os_buddy.block_group[i]];
				_os_buddy.block_group[i] = GROUP_COUNT;
			}
		}
	}
	return size;
}
