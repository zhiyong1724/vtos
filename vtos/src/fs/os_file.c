#include "fs/os_file.h"
#include "base/os_string.h"
#include "vtos.h"

uint32 little_file_data_write(uint32 id, uint64 index, void *data, uint32 size)
{
	if (size == FS_CLUSTER_SIZE)
	{
		cluster_write(id, data);
	}
	else
	{
		uint8 *buff = (uint8 *)os_kmalloc(FS_CLUSTER_SIZE);
		cluster_read(id, buff);
		os_mem_cpy(&buff[index], data, size);
		cluster_write(id, buff);
		os_kfree(buff);
	}
	return size;
}

uint32 little_file_data_read(uint32 id, uint64 index, void *data, uint32 size)
{
	uint8 *buff = (uint8 *)os_kmalloc(FS_CLUSTER_SIZE);
	cluster_read(id, buff);
	os_mem_cpy(data, &buff[index], size);
	os_kfree(buff);
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
		uint32 *p = (uint32 *)os_kmalloc(FS_CLUSTER_SIZE);
		for (i = 0; i < FS_MAX_INDEX_NUM; i++)
		{
			p[i] = convert_endian(data[i]);
		}
		cluster_write(cluster_id, (uint8 *)p);
		os_kfree(p);
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
	uint32 *buff = (uint32 *)os_kmalloc(FS_CLUSTER_SIZE);
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
	os_kfree(buff);
	return id;
}

static uint32 write_to_tree(uint32 tree, uint32 tree_height, uint32 *count, uint64 *index, uint8 **data, uint32 *size)
{
	uint32 j = (uint32)(*index % FS_CLUSTER_SIZE);
	if (1 == tree_height)
	{
		uint32 len = FS_CLUSTER_SIZE - j;
		if (*size < len)
		{
			len = *size;
		}
		if (*data != NULL)
		{
			little_file_data_write(tree, j, *data, len);
			*data += len;
		}
		*index += len;
		*size -= len;
	}
	else
	{
		uint32 k;
		uint32 i = (uint32)(*index / FS_CLUSTER_SIZE);
		uint32 *list = NULL;
		for (k = 1; k < tree_height - 1; k++)
		{
			i = i / FS_MAX_INDEX_NUM;
		}
		i = i % FS_MAX_INDEX_NUM;
		list = (uint32 *)os_kmalloc(FS_CLUSTER_SIZE);
		cluster_list_read(tree, list);
		for (; *size > 0 && i < FS_MAX_INDEX_NUM; i++)
		{
			if (0 == list[i])
			{
				uint32 *empty = (uint32 *)os_kmalloc(FS_CLUSTER_SIZE);
				init_index_node(empty);
				list[i] = cluster_alloc();
				(*count)++;
				cluster_list_write(list[i], empty);
				os_kfree(empty);
			}
			write_to_tree(list[i], tree_height - 1, count, index, data, size);
		}
		cluster_list_write(tree, list);
		os_kfree(list);
	}
	return tree;
}

static uint32 data_cluster_count(uint32 tree_height)
{
	uint32 count = 1;
	for (; tree_height > 1; tree_height--)
	{
		count *= FS_MAX_INDEX_NUM;
	}
	return count;
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
			uint32 i = (uint32)(index / FS_CLUSTER_SIZE);
			if (i >= data_cluster_count(tree_height))
			{
				id = grow(id, 1);
				(*count)++;
				tree_height++;
			}
			write_to_tree(id, tree_height, count, &index, &data, &size);
		}
		
	}
	return id;
}

static uint32 read_from_tree(uint32 tree, uint32 tree_height, uint64 *index, uint8 **data, uint32 *size)
{
	uint32 j = (uint32)(*index % FS_CLUSTER_SIZE);
	if (1 == tree_height)
	{
		uint32 len = FS_CLUSTER_SIZE - j;
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
		uint32 k;
		uint32 i = (uint32)(*index / FS_CLUSTER_SIZE);
		uint32 *list = NULL;
		for (k = 1; k < tree_height - 1; k++)
		{
			i = i / FS_MAX_INDEX_NUM;
		}
		i = i % FS_MAX_INDEX_NUM;
		list = (uint32 *)os_kmalloc(FS_CLUSTER_SIZE);
		cluster_list_read(tree, list);
		for (; *size > 0 && i < FS_MAX_INDEX_NUM; i++)
		{
			read_from_tree(list[i], tree_height - 1, index, data, size);
		}
		os_kfree(list);
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
		uint32 *list = (uint32 *)os_kmalloc(FS_CLUSTER_SIZE);
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
		os_kfree(list);
	}
}

void file_data_remove(uint32 id, uint32 count)
{
	uint32 tree_height = calculate_tree_height(count);
	do_file_data_remove(id, tree_height);
}