#include "fs/os_file.h"
#include "lib/os_string.h"
#include "vtos.h"
#include <stdlib.h>

uint32 little_file_data_write(uint32 id, uint32 index, void *data, uint32 size)
{
	uint32 i = 0;
	if (index + size <= FS_PAGE_SIZE)
	{
		if (size == FS_PAGE_SIZE)
		{
			if (CLUSTER_NONE == cluster_write(id, data))
			{
				i = size;
			}
		}
		else
		{
			uint8 *buff = (uint8 *)malloc(FS_PAGE_SIZE);
			if (CLUSTER_NONE == cluster_read(id, buff))
			{
				os_mem_cpy(&buff[index], data, size);
				if (CLUSTER_NONE == cluster_write(id, buff))
				{
					i = size;
				}
			}
			free(buff);
		}
	}
	return i;
}

uint32 little_file_data_read(uint32 id, uint32 index, void *data, uint32 size)
{
	uint32 i = 0;
	if (index + size <= FS_PAGE_SIZE)
	{
		uint8 *buff = (uint8 *)malloc(FS_PAGE_SIZE);
		if (CLUSTER_NONE == cluster_read(id, data))
		{
			os_mem_cpy(data, &buff[index], size);
			i = size;
		}
		free(buff);
	}
	return i;
}

static uint32 cluster_dir_read(uint32 cluster_id, uint32 *data)
{
	uint32 ret;
	if (is_little_endian())
	{
		ret = cluster_read(cluster_id, (uint8 *)data);
	}
	else
	{
		uint32 i;
		ret = cluster_read(cluster_id, (uint8 *)data);
		for (i = 0; i < FS_PAGE_SIZE / sizeof(uint32); i++)
		{
			data[i] = convert_endian(data[i]);
		}
		
	}
	return ret;
}

static uint32 cluster_dir_write(uint32 cluster_id, uint32 *data)
{
	uint32 ret;
	if (is_little_endian())
	{
		ret = cluster_write(cluster_id, (uint8 *)data);
	}
	else
	{
		uint32 i;
		uint32 *p = (uint32 *)malloc(FS_PAGE_SIZE);
		for (i = 0; i < FS_PAGE_SIZE / sizeof(uint32); i++)
		{
			p[i] = convert_endian(data[i]);
		}
		ret = cluster_write(cluster_id, (uint8 *)p);
		free(p);
	}
	return ret;
}

uint32 create_file_data()
{
	uint32 dir_id = cluster_alloc();
	if (dir_id != 0)
	{
		uint32 data_id = cluster_alloc();
		if (data_id != 0)
		{
			uint32 i;
			uint32 *dir = (uint32 *)malloc(FS_PAGE_SIZE);
			dir[0] = 0;
			dir[1] = data_id;
			for (i = 2; i < FS_PAGE_SIZE / sizeof(uint32); i++)
			{
				dir[i] = 0;
			}
			if (cluster_dir_write(dir_id, dir) == CLUSTER_NONE)
			{
				cluster_free(dir_id);
				cluster_free(data_id);
				dir_id = 0;
			}
			free(dir);
		}
		else
		{
			cluster_free(dir_id);
			return 0;
		}
	}
	return dir_id;
}

uint32 get_data_id(uint32 *first, uint32 *i, uint32 is_create)
{
	uint32 id = 0;
	if (*i < FS_PAGE_SIZE / sizeof(uint32) - 1)
	{
		id = first[*i + 1];
		if (is_create && 0 == id)
		{
			id = cluster_alloc();
		}
		*i++;
	}
	else
	{
		if (*first != 0)
		{
			if (cluster_dir_read(*first, first) == CLUSTER_NONE)
			{
				*i = 0;
				id = first[*i + 1];
				*i++;
			}
		}
		else if (is_create)
		{
			*first = create_file_data();
			if (*first != 0)
			{
				if (cluster_dir_read(*first, first) == CLUSTER_NONE)
				{
					*i = 0;
					id = first[*i + 1];
					*i++;
				}
			}
		}
	}
	return id;
}

uint32 file_data_write(uint32 id, uint64 index, uint8 *data, uint32 size)
{
	uint32 flag = 0;
	uint32 cur_size = 0;
	uint32 *dir = (uint32 *)malloc(FS_PAGE_SIZE);
	uint8 *buff = (uint8 *)malloc(FS_PAGE_SIZE);
	uint32 i = (uint32)(index / FS_PAGE_SIZE);
	uint32 i1 = index % FS_PAGE_SIZE;
	uint32 j = i / (FS_PAGE_SIZE - sizeof(uint32));
	uint32 j1 = i % (FS_PAGE_SIZE - sizeof(uint32));
	for (;; j--)
	{
		//找出目录页
		if (cluster_dir_read(id, dir) != CLUSTER_NONE)
		{
			break;
		}
		id = *dir;
		if (0 == j)
		{
			flag = 1;
			break;
		}
	}
	if (flag)
	{
		flag = 0;
		id = get_data_id(dir, &j1, 1);
		if (id > 0)
		{
			if (cluster_read(id, buff) == CLUSTER_NONE)
			{
				flag = 1;
			}
		}
	}
	if (flag)
	{
		uint32 temp;
		flag = 0;
		temp = FS_PAGE_SIZE - i1;
		if (size < temp)
		{
			temp = size;
		}
		os_mem_cpy(&buff[i1], data, temp);
		if (cluster_write(id, buff) == CLUSTER_NONE)
		{
			flag = 1;
			cur_size = temp;
		}
	}
	if (flag)
	{
		for (; cur_size < size;)
		{
			id = get_data_id(dir, &j1, 1);
			if (id > 0)
			{
				if (cur_size + FS_PAGE_SIZE <= size)
				{
					if (cluster_write(id, &data[cur_size]) != CLUSTER_NONE)
					{
						break;
					}
					cur_size += FS_PAGE_SIZE;
				}
				else
				{
					if (cluster_read(id, buff) != CLUSTER_NONE)
					{
						break;
					}
					os_mem_cpy(buff, &data[cur_size], size - cur_size);
					if (cluster_write(id, buff) != CLUSTER_NONE)
					{
						break;
					}
					cur_size = size;
				}
				
			}
		}
	}
	free(dir);
	free(buff);
	return cur_size;
}

uint32 file_data_read(uint32 id, uint64 index, uint8 *data, uint32 size)
{
	uint32 flag = 0;
	uint32 cur_size = 0;
	uint32 *dir = (uint32 *)malloc(FS_PAGE_SIZE);
	uint8 *buff = (uint8 *)malloc(FS_PAGE_SIZE);
	uint32 i = (uint32)(index / FS_PAGE_SIZE);
	uint32 i1 = index % FS_PAGE_SIZE;
	uint32 j = i / (FS_PAGE_SIZE - sizeof(uint32));
	uint32 j1 = i % (FS_PAGE_SIZE - sizeof(uint32));
	for (;; j--)
	{
		//找出目录页
		if (cluster_dir_read(id, dir) != CLUSTER_NONE)
		{
			break;
		}
		id = *dir;
		if (0 == j)
		{
			flag = 1;
			break;
		}
	}
	if (flag)
	{
		flag = 0;
		id = get_data_id(dir, &j1, 0);
		if (id > 0)
		{
			if (cluster_read(id, buff) == CLUSTER_NONE)
			{
				flag = 1;
			}
		}
	}
	if (flag)
	{
		cur_size = FS_PAGE_SIZE - i1;
		if (size < cur_size)
		{
			cur_size = size;
		}
		os_mem_cpy(data, &buff[i1], cur_size);
		for (; cur_size < size;)
		{
			id = get_data_id(dir, &j1, 0);
			if (id > 0)
			{
				if (cluster_read(id, buff) != CLUSTER_NONE)
				{
					break;
				}
				if (cur_size + FS_PAGE_SIZE <= size)
				{
					if (cluster_read(id, &data[cur_size]) != CLUSTER_NONE)
					{
						break;
					}
					cur_size += FS_PAGE_SIZE;
				}
				else
				{
					if (cluster_read(id, buff) != CLUSTER_NONE)
					{
						break;
					}
					os_mem_cpy(&data[cur_size], buff, size - cur_size);
					cur_size = size;
				}
			}
			else
			{
				break;
			}
		}
	}
	return cur_size;
}

void file_data_remove(uint32 id)
{
	uint32 *dir = (uint32 *)malloc(FS_PAGE_SIZE);
	uint32 i;
	do 
	{
		if (cluster_dir_read(id, dir) != CLUSTER_NONE)
		{
			break;
		}
		id = dir[0];
		for (i = 0; i < FS_PAGE_SIZE; i++)
		{
			cluster_free(dir[i]);
		}
	} while (id > 0);
	free(dir);
}