#include "fs/os_file.h"
#include "lib/os_string.h"
#include "vtos.h"
#include <stdlib.h>

uint32 little_file_data_write(uint32 id, uint64 index, void *data, uint32 size)
{
	if (size == FS_PAGE_SIZE)
	{
		cluster_write(id, data);
	}
	else
	{
		uint8 *buff = (uint8 *)malloc(FS_PAGE_SIZE);
		cluster_read(id, buff);
		os_mem_cpy(&buff[index], data, size);
		cluster_write(id, buff);
		free(buff);
	}
	return size;
}

uint32 little_file_data_read(uint32 id, uint64 index, void *data, uint32 size)
{
	uint8 *buff = (uint8 *)malloc(FS_PAGE_SIZE);
	cluster_read(id, buff);
	os_mem_cpy(data, &buff[index], size);
	free(buff);
	return size;
}

static void cluster_list_read(uint32 cluster_id, uint32 *data)
{
	if (is_little_endian())
	{
		cluster_read(cluster_id, (uint8 *)data);
	}
	else
	{
		uint32 i;
		cluster_read(cluster_id, (uint8 *)data);
		for (i = 0; i < FS_PAGE_SIZE / sizeof(uint32); i++)
		{
			data[i] = convert_endian(data[i]);
		}

	}
}

static void cluster_list_write(uint32 cluster_id, uint32 *data)
{
	if (is_little_endian())
	{
		cluster_write(cluster_id, (uint8 *)data);
	}
	else
	{
		uint32 i;
		uint32 *p = (uint32 *)malloc(FS_PAGE_SIZE);
		for (i = 0; i < FS_PAGE_SIZE / sizeof(uint32); i++)
		{
			p[i] = convert_endian(data[i]);
		}
		cluster_write(cluster_id, (uint8 *)p);
		free(p);
	}
}

uint32 create_file_list(uint32 data_id)
{
	uint32 list_id = cluster_alloc();
	uint32 i;
	uint32 *list = (uint32 *)malloc(FS_PAGE_SIZE);
	list[0] = 0;
	list[1] = data_id;
	for (i = 2; i < FS_PAGE_SIZE / sizeof(uint32); i++)
	{
		list[i] = 0;
	}
	cluster_list_write(list_id, list);
	free(list);
	return list_id;
}

uint32 get_data_id(uint32 *first, uint32 *i)
{
	uint32 id = 0;
	if (*i < FS_PAGE_SIZE / sizeof(uint32) - 1)
	{
		id = first[*i + 1];
		if (0 == id)
		{
			id = cluster_alloc();
		}
		*i++;
	}
	else
	{
		if (*first != 0)
		{
			cluster_dir_read(*first, first);
			*i = 0;
			id = first[*i + 1];
			*i++;
		}
		else
		{
			*first = create_file_data();
			if (*first != 0)
			{
				cluster_list_read(*first, first);
				*i = 0;
				id = first[*i + 1];
				*i++;
			}
		}
	}
	return id;
}

uint32 file_data_write(uint32 id, uint64 index, uint8 *data, uint32 size)
{
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
		cluster_list_read(id, dir);
		id = *dir;
		if (0 == j)
		{
			break;
		}
	}
	id = get_data_id(dir, &j1, 1);
	if (id > 0)
	{
		cluster_read(id, buff);
	}
	uint32 temp;
	temp = FS_PAGE_SIZE - i1;
	if (size < temp)
	{
		temp = size;
	}
	os_mem_cpy(&buff[i1], data, temp);
	cluster_write(id, buff);
	cur_size = temp;
	for (; cur_size < size;)
	{
		id = get_data_id(dir, &j1, 1);
		if (id > 0)
		{
			if (cur_size + FS_PAGE_SIZE <= size)
			{
				cluster_write(id, &data[cur_size]);
				cur_size += FS_PAGE_SIZE;
			}
			else
			{
				cluster_read(id, buff);
				os_mem_cpy(buff, &data[cur_size], size - cur_size);
				cluster_write(id, buff);
				cur_size = size;
			}

		}
	}
	free(dir);
	free(buff);
	return cur_size;
}

uint32 file_data_read(uint32 id, uint64 index, uint8 *data, uint32 size)
{
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
		cluster_list_read(id, dir);
		id = *dir;
		if (0 == j)
		{
			break;
		}
	}
	cluster_read(id, buff);
	id = get_data_id(dir, &j1, 0);
	if (id > 0)
	{
		cluster_read(id, buff);
	}
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
			cluster_read(id, buff);
			if (cur_size + FS_PAGE_SIZE <= size)
			{
				cluster_read(id, &data[cur_size]);
				cur_size += FS_PAGE_SIZE;
			}
			else
			{
				cluster_read(id, buff);
				os_mem_cpy(&data[cur_size], buff, size - cur_size);
				cur_size = size;
			}
		}
		else
		{
			break;
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
		cluster_list_read(id, dir);
		id = dir[0];
		for (i = 0; i < FS_PAGE_SIZE; i++)
		{
			cluster_free(dir[i]);
		}
	} while (id > 0);
	free(dir);
}