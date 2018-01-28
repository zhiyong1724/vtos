#include "os_mem.h"
#include "lib/os_string.h"
#include "vtos.h"
void *_start_addr = NULL;
void *_end_addr = NULL;
static struct mem_controler _mem_controler;
static const os_size_t _pow2x[GROUP_COUNT] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };

static void insert_mem_block(os_size_t group, struct block_node *new_block)
{
	new_block->pre_block = NULL;
	if (NULL == _mem_controler.index_array[group])
	{
		new_block->next_block = NULL;
	}
	else
	{
		new_block->next_block = _mem_controler.index_array[group];
		_mem_controler.index_array[group]->pre_block = new_block;
	}
	_mem_controler.index_array[group] = new_block;
}

static os_size_t format_mem(uint8 *start_addr, os_size_t group, os_size_t addr_space)
{
	os_size_t block_size = PAGE_SIZE * _pow2x[group];
	if (addr_space >= block_size)
	{
		os_size_t count = addr_space / block_size;
		os_size_t i = 0;
		for (i = 0; i < count; i++)
		{
			insert_mem_block(group, (struct block_node *)start_addr);
			start_addr += block_size;
			addr_space -= block_size;
			_mem_controler.total_mem += block_size;
		}
		if (group < _mem_controler.max_group && count % 2 != 0)
		{
			os_size_t offset = 0;
			os_size_t x = 0;
			os_size_t y = 0;
			uint8 mark_bit = 0x80;
			offset = ((uint8 *)start_addr - (uint8 *)_start_addr - block_size)
				/ _mem_controler.size_array[group + 1];
			x = offset >> 3;
			y = offset % 8;
			mark_bit >>= y;
			_mem_controler.block_bitmap_array[group][x] ^= mark_bit;
		}
	}
	if (group > 0)
	{
		group--;
		addr_space = format_mem(start_addr, group, addr_space);
	}
	return addr_space;
}

static void mem_controler_init(uint8 *index, os_size_t page_count)
{
	os_size_t size = page_count;
	_mem_controler.total_mem = 0;
	_mem_controler.used_mem = 0;
	_mem_controler.block_group = index;
	os_mem_set(_mem_controler.block_group, GROUP_COUNT, size);  //初始化block_group
	index += size;
	{
		os_size_t i = 0;
		size = (((page_count >> 1) + 1) >> 3) + 1;
		for (i = 0; i < GROUP_COUNT - 1; i++)
		{
			//初始化block_bitmap
			if (i < _mem_controler.max_group)
			{
				_mem_controler.block_bitmap_array[i] = index;
				os_mem_set(_mem_controler.block_bitmap_array[i], 0, size);
				index += size;
				size = (size >> 1) + 1;
			}
			else
			{
				_mem_controler.block_bitmap_array[i] = NULL;
			}
		}
		for (i = 0; i < GROUP_COUNT; i++)
		{
			//初始化index_array
			_mem_controler.index_array[i] = NULL;
		}
		for (i = 0; i < GROUP_COUNT; i++)
		{
			//初始化size_array
			_mem_controler.size_array[i] = PAGE_SIZE * _pow2x[i];
		}
	}
}

static os_size_t create_mem_controler(os_size_t group)
{
	os_size_t ret = 1;
	uint8 *start_addr = (uint8 *)_start_addr;
	uint8 *end_addr = (uint8 *)_end_addr;
	os_size_t block_size = PAGE_SIZE * _pow2x[group];
	os_size_t addr_space = (os_size_t)(end_addr - start_addr);
	if (block_size < addr_space)
	{ 
		os_size_t i = 0;
		os_size_t page_count = addr_space / PAGE_SIZE;
		os_size_t mem_controler_size = page_count; //block_group空间，每个字节记录一页
		os_size_t block_bitmap_size = (((page_count >> 1) + 1) >> 3) + 1;         //block_bitmap_0空间，加一是为了防止除不尽，空间留多一点无妨
		mem_controler_size += block_bitmap_size;
		for(i = 1; i < group; i++)
		{
			block_bitmap_size = (block_bitmap_size >> 1) + 1;
			mem_controler_size += block_bitmap_size;
		}
		if(addr_space > block_size + mem_controler_size)
		{
			//到这里表示地址空间够用，可以初始化这一组
			_mem_controler.max_group = group;
			addr_space -= mem_controler_size;
			mem_controler_init(end_addr - mem_controler_size, page_count);
			format_mem(start_addr, group, addr_space);
			ret = 0;
		}
	}
	return ret;
}

os_size_t os_mem_init(void)
{
	os_size_t ret = 1;
	os_size_t i = GROUP_COUNT - 1;
	os_get_start_addr();
	if (_start_addr != NULL && _end_addr > _start_addr)
	{
		for (; i >= 0; i--)
		{
			ret = create_mem_controler(i);
			if (0 == ret)
			{
				break;
			}
		}
	}
	return ret;
}

static void delete_mem_block(os_size_t group, struct block_node *block)
{
	if (NULL == block->pre_block && NULL == block->next_block)
	{
		_mem_controler.index_array[group] = NULL;
	}
	else if (NULL == block->pre_block && block->next_block != NULL)
	{
		block->next_block->pre_block = NULL;
		_mem_controler.index_array[group] = block->next_block;
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
	os_cpu_sr cpu_sr;
	void *ret = NULL;
	while (1)
	{
		cpu_sr = os_cpu_sr_save();
		if (group <= _mem_controler.max_group)
		{
			if (_mem_controler.index_array[group] != NULL)
			{
				ret = _mem_controler.index_array[group];
				delete_mem_block(group, _mem_controler.index_array[group]);
				if (group < _mem_controler.max_group)
				{
					os_size_t offset = 0;
					os_size_t i = 0;
					os_size_t j = 0;
					uint8 mark_bit = 0x80;
					offset = ((uint8 *)ret - (uint8 *)_start_addr)
						/ _mem_controler.size_array[group + 1];
					i = offset >> 3;
					j = offset % 8;
					mark_bit >>= j;
					_mem_controler.block_bitmap_array[group][i] ^= mark_bit;
				}
			}
			else
			{
				ret = get_block(group + 1);
				if (ret != NULL)
				{
					void *next_addr = (uint8 *)ret + _mem_controler.size_array[group];
					insert_mem_block(group, (struct block_node *) next_addr);
					if (group < _mem_controler.max_group)
					{
						os_size_t offset = 0;
						os_size_t i = 0;
						os_size_t j = 0;
						uint8 mark_bit = 0x80;
						offset = ((uint8 *)ret - (uint8 *)_start_addr)
							/ _mem_controler.size_array[group + 1];
						i = offset >> 3;
						j = offset % 8;
						mark_bit >>= j;
						_mem_controler.block_bitmap_array[group][i] ^= mark_bit;
					}
				}
			}
		}
		if (NULL == ret)
		{
			os_cpu_sr_restore(cpu_sr);
			os_sleep(1000);
		}
		break;
	}
	os_cpu_sr_restore(cpu_sr);
	return ret;
}

static void free_block(os_size_t group, void *addr)
{
	if (group < _mem_controler.max_group)
	{
		//不是最后一组
		os_size_t offset = ((uint8 *)addr - (uint8 *)_start_addr)
			/ _mem_controler.size_array[group + 1];
		os_size_t i = offset >> 3;
		os_size_t j = offset % 8;
		uint8 mark_bit = 0x80;
		mark_bit >>= j;
		mark_bit &= _mem_controler.block_bitmap_array[group][i];
		if (mark_bit > 0)
		{
			//表示可以合并
			if (0
				== ((uint8 *)addr - (uint8 *)_start_addr)
				% _mem_controler.size_array[group + 1])
			{
				//释放后面的伙伴
				delete_mem_block(group,
					(struct block_node *) ((uint8 *)addr
					+ _mem_controler.size_array[group]));
				free_block(group + 1, addr);
			}
			else
			{
				//释放前面的伙伴
				delete_mem_block(group,
					(struct block_node *) ((uint8 *)addr
					- _mem_controler.size_array[group]));
				free_block(group + 1, (uint8 *)addr - _mem_controler.size_array[group]);
			}
		}
		else
		{
			//不可以合并就直接插入链表中
			insert_mem_block(group, (struct block_node *) addr);
		}
		mark_bit = 0x80;
		mark_bit >>= j;
		_mem_controler.block_bitmap_array[group][i] ^= mark_bit;
	}
	else
	{
		//最后一组直接插入链表中
		insert_mem_block(group, (struct block_node *) addr);
	}
}

void *os_kmalloc(os_size_t size)
{
	os_cpu_sr cpu_sr;
	void *ret = NULL;
	if (size <= _mem_controler.size_array[0])
	{
		ret = get_block(0);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 0;
			_mem_controler.used_mem += _mem_controler.size_array[0];
		}
	}
	else if (size <= _mem_controler.size_array[1])
	{
		ret = get_block(1);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 1;
			_mem_controler.used_mem += _mem_controler.size_array[1];
		}
	}
	else if (size <= _mem_controler.size_array[2])
	{
		ret = get_block(2);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 2;
			_mem_controler.used_mem += _mem_controler.size_array[2];
		}
	}
	else if (size <= _mem_controler.size_array[3])
	{
		ret = get_block(3);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 3;
			_mem_controler.used_mem += _mem_controler.size_array[3];
		}
	}
	else if (size <= _mem_controler.size_array[4])
	{
		ret = get_block(4);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 4;
			_mem_controler.used_mem += _mem_controler.size_array[4];
		}
	}
	else if (size <= _mem_controler.size_array[5])
	{
		ret = get_block(5);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 5;
			_mem_controler.used_mem += _mem_controler.size_array[5];
		}
	}
	else if (size <= _mem_controler.size_array[6])
	{
		ret = get_block(6);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 6;
			_mem_controler.used_mem += _mem_controler.size_array[6];
		}
	}
	else if (size <= _mem_controler.size_array[7])
	{
		ret = get_block(7);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 7;
			_mem_controler.used_mem += _mem_controler.size_array[7];
		}
	}
	else if (size <= _mem_controler.size_array[8])
	{
		ret = get_block(8);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 8;
			_mem_controler.used_mem += _mem_controler.size_array[8];
		}
	}
	else if (size <= _mem_controler.size_array[9])
	{
		ret = get_block(9);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 9;
			_mem_controler.used_mem += _mem_controler.size_array[9];
		}
	}
	else if (size <= _mem_controler.size_array[10])
	{
		ret = get_block(10);
		if (ret != NULL)
		{
			os_size_t i = ((uint8 *)ret - (uint8 *)_start_addr) / _mem_controler.size_array[0];
			cpu_sr = os_cpu_sr_save();
			_mem_controler.block_group[i] = 10;
			_mem_controler.used_mem += _mem_controler.size_array[10];
		}
	}
	os_error_msg("Memory allocation failure.", 0);
	os_cpu_sr_restore(cpu_sr);
	return ret;
}

void os_kfree(void *addr)
{
	os_cpu_sr cpu_sr = os_cpu_sr_save();
	if (addr >= (void *)_start_addr)
	{
		os_size_t addr_offset = (uint8 *)addr - (uint8 *)_start_addr;
		if (0 == addr_offset % _mem_controler.size_array[0])
		{
			//有效的地址
			os_size_t i = addr_offset / _mem_controler.size_array[0];
			free_block(_mem_controler.block_group[i], addr);
			_mem_controler.used_mem -= _mem_controler.size_array[_mem_controler.block_group[i]];
			_mem_controler.block_group[i] = GROUP_COUNT;
		}
	}
	os_cpu_sr_restore(cpu_sr);
}

os_size_t os_total_mem_size(void)
{
	return _mem_controler.total_mem;
}

os_size_t os_used_mem_size(void)
{
	return _mem_controler.used_mem;
}
