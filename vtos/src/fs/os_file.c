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
		for (i = 0; i < FS_MAX_INDEX_NUM; i++)
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
		for (i = 0; i < FS_MAX_INDEX_NUM; i++)
		{
			p[i] = convert_endian(data[i]);
		}
		cluster_write(cluster_id, (uint8 *)p);
		free(p);
	}
}

static void init_index_node(uint32 *node)
{
	uint32 i;
	for (i = 0; i < FS_MAX_INDEX_NUM; i++)
	{
		node[i] = 0;
	}
}

static uint32 calculate_tree_height(uint32 node_num)
{
	uint32 height = 0;
	uint32 cur = 1;
	uint32 total = 1;
	while (node_num >= total)
	{
		cur *= FS_MAX_INDEX_NUM;
		total += cur;
		height++;
	}
	node_num = node_num - total + cur;
	if (node_num > 0)
	{
		height++;
	}
	return height;
}

static uint32 grow(uint32 id, uint32 is_list)
{
	uint32 new_id;
	uint32 *buff = (uint32 *)malloc(FS_PAGE_SIZE);
	if (is_list)
	{
		cluster_list_read(id, buff);
	}
	else
	{
		cluster_read(id, (uint8 *)buff);
	}
	new_id = cluster_alloc();
	if (is_list)
	{
		cluster_list_write(new_id, buff);
	}
	else
	{
		cluster_write(new_id, (uint8 *)buff);
	}
	
	init_index_node(buff);
	*buff = new_id;
	cluster_list_write(id, buff);
	free(buff);
	return id;
}

static uint32 write_to_tree(uint32 tree, uint32 tree_height, uint32 *count, uint64 *index, uint8 **data, uint32 *size)
{
	uint32 i = (uint32)(*index / FS_PAGE_SIZE);
	uint32 j = (uint32)(*index % FS_PAGE_SIZE);
	uint32 k;
	for (k = 1; k < tree_height - 1; k++)
	{
		i = i / FS_MAX_INDEX_NUM;
	}
	if (k < tree_height)
	{
		i = i % FS_MAX_INDEX_NUM;
	}
	if (1 == tree_height)
	{
		uint32 len = FS_PAGE_SIZE - j;
		if (*size < len)
		{
			len = *size;
		}
		little_file_data_write(tree, j, *data, len);
		*data += len;
		*index += len;
		*size -= len;
	}
	else
	{
		uint32 *list = (uint32 *)malloc(FS_PAGE_SIZE);
		cluster_list_read(tree, list);
		while (*size > 0 && i < FS_MAX_INDEX_NUM)
		{
			if (0 == list[i])
			{
				uint32 *empty = (uint32 *)malloc(FS_PAGE_SIZE);
				init_index_node(empty);
				list[i] = cluster_alloc();
				(*count)++;
				cluster_list_write(list[i], empty);
				free(empty);
			}
			write_to_tree(list[i], tree_height - 1, count, index, data, size);
			i++;
		}
		cluster_list_write(tree, list);
		free(list);
	}
	return tree;
}

uint32 file_data_write(uint32 id, uint32 *count, uint64 index, uint8 *data, uint32 size)
{
	if (1 == *count)
	{
		id = grow(id, 0);
		(*count)++;
	}
	{
		uint32 tree_height = calculate_tree_height(*count);
		while (size > 0)
		{
			write_to_tree(id, tree_height, count, &index, &data, &size);
			if (size > 0)
			{
				id = grow(id, 1);
				(*count)++;
				tree_height++;
			}
		}
		
	}
	return id;
}

static uint32 read_from_tree(uint32 tree, uint32 tree_height, uint64 *index, uint8 **data, uint32 *size)
{
	uint32 i = (uint32)(*index / FS_PAGE_SIZE);
	uint32 j = (uint32)(*index % FS_PAGE_SIZE);
	uint32 k;
	for (k = 1; k < tree_height - 1; k++)
	{
		i = i / FS_MAX_INDEX_NUM;
	}
	if (k < tree_height)
	{
		i = i % FS_MAX_INDEX_NUM;
	}
	if (1 == tree_height)
	{
		uint32 len = FS_PAGE_SIZE - j;
		if (*size < len)
		{
			len = *size;
		}
		little_file_data_read(tree, j, *data, len);
		*data += len;
		*index += len;
		*size -= len;
	}
	else
	{
		uint32 *list = (uint32 *)malloc(FS_PAGE_SIZE);
		cluster_list_read(tree, list);
		while (*size > 0 && i < FS_MAX_INDEX_NUM)
		{
			read_from_tree(list[i], tree_height - 1, index, data, size);
			i++;
		}
		free(list);
	}
	return tree;
}

uint32 file_data_read(uint32 id, uint32 count, uint64 index, uint8 *data, uint32 size)
{
	uint32 tree_height = calculate_tree_height(count);
	read_from_tree(id, tree_height, &index, &data, &size);
	return id;
}

static void do_file_data_remove(uint32 id, uint32 height)
{
	if (1 == height)
	{
		cluster_free(id);
	}
	else
	{
		uint32 *list = (uint32 *)malloc(FS_PAGE_SIZE);
		uint32 i;
		cluster_list_read(id, list);
		for (i = 0; i < FS_MAX_INDEX_NUM; i++)
		{
			if (list[i] != 0)
			{
				do_file_data_remove(list[i], height - 1);
			}
			else
			{
				break;
			}
		}
		cluster_free(id);
		free(list);
	}
}

void file_data_remove(uint32 id, uint32 count)
{
	uint32 tree_height = calculate_tree_height(count);
	do_file_data_remove(id, tree_height);
}