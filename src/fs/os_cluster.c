#include "fs/os_cluster.h"
#include "base/os_string.h"
#include "base/os_bitmap_index.h"
#include "vtos.h"
#include "fs/os_journal.h"
uint32 convert_endian(uint32 src)
{
	uint32 dest;
	uint8 *pdest;
	uint8 *psrc;
	uint32 i = 0;
	pdest = (uint8 *)(&dest) + sizeof(uint32) - 1;
	psrc = (uint8 *)(&src);
	for (; i < sizeof(uint32); i++, pdest--)
	{
		*pdest = psrc[i];
	}
	return dest;
}

uint64 convert_endian64(uint64 src)
{
	uint64 dest;
	uint8 *pdest;
	uint8 *psrc;
	uint64 i = 0;
	pdest = (uint8 *)(&dest) + sizeof(uint64) - 1;
	psrc = (uint8 *)(&src);
	for (; i < sizeof(uint64); i++, pdest--)
	{
		*pdest = psrc[i];
	}
	return dest;
}

static uint8 *bitmap_load(uint32 id, os_cluster *cluster)
{
	uint8 *data = NULL;
	os_map_iterator *itr = os_map_find(&cluster->bitmaps, &id);
	if (itr != NULL)
	{
		uint8 **pp = (uint8 **)os_map_second(&cluster->bitmaps, itr);
		data = *pp;
	}
	else
	{
		data = (uint8 *)os_malloc(FS_CLUSTER_SIZE);
		cluster_read(FIRST_CLUSTER_MANAGER_ID + 1 + id, data, cluster);
		os_map_insert(&cluster->bitmaps, &id, &data);
	}
	return data;
}

static void bitmap_flush(uint32 id, uint8 *data, os_cluster *cluster)
{
	cluster_write(FIRST_CLUSTER_MANAGER_ID + 1 + id, data, cluster);
}

static void bitmaps_flush(os_cluster *cluster)
{
	os_map_iterator *itr = os_map_begin(&cluster->bitmaps);
	for (; itr != NULL; (itr = os_map_next(&cluster->bitmaps, itr)))
	{
		uint32 *key = (uint32 *)os_map_first(itr);
		uint8 **pp = (uint8 **)os_map_second(&cluster->bitmaps, itr);
		bitmap_flush(*key, *pp, cluster);
		if (*key != cluster->cache_id)
		{
			os_free(*pp);
		}
	}
	os_map_clear(&cluster->bitmaps);
	bitmap_flush(cluster->cache_id, cluster->bitmap, cluster);
}

static void cluster_manager_flush(os_cluster *cluster)
{
	if (is_little_endian())
	{
		cluster_write(FIRST_CLUSTER_MANAGER_ID, (uint8 *)cluster->pcluster_manager, cluster);
	}
	else
	{
		struct cluster_manager *temp = (struct cluster_manager *)os_malloc(FS_CLUSTER_SIZE);
		temp->cur_index = convert_endian(cluster->pcluster_manager->cur_index);
		temp->used_cluster_count = convert_endian(cluster->pcluster_manager->used_cluster_count);
		cluster_write(FIRST_CLUSTER_MANAGER_ID, (uint8 *)temp, cluster);
		os_free(temp);
	}
}

void cluster_manager_load(os_cluster *cluster)
{
	if (is_little_endian())
	{
		cluster_read(FIRST_CLUSTER_MANAGER_ID, (uint8 *)cluster->pcluster_manager, cluster);
	}
	else
	{
		cluster_read(FIRST_CLUSTER_MANAGER_ID, (uint8 *)cluster->pcluster_manager, cluster);
		cluster->pcluster_manager->cur_index = convert_endian(cluster->pcluster_manager->cur_index);
		if (cluster->pcluster_manager->cur_index >= cluster->total_cluster_count)
		{
			cluster->pcluster_manager->cur_index = 0;
		}
		cluster->pcluster_manager->used_cluster_count = convert_endian(cluster->pcluster_manager->used_cluster_count);
	}
}

static void bitmap_init(uint32 id, os_cluster *cluster)
{
	uint8 *temp = (uint8 *)os_malloc(FS_CLUSTER_SIZE);
	os_mem_set(temp, 0, FS_CLUSTER_SIZE);
	bitmap_flush(id, temp, cluster);
	os_free(temp);
}

void os_cluster_init(os_cluster *cluster, uint32 dev_id)
{
	cluster->dev_id = dev_id;
	cluster->pcluster_manager = (struct cluster_manager *)os_malloc(FS_CLUSTER_SIZE);
	cluster->bitmap = NULL;
	cluster->cache_id = 0;
	os_get_disk_info(&cluster->dinfo, dev_id);
	if (cluster->dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		cluster->divisor = FS_CLUSTER_SIZE / cluster->dinfo.page_size;
		for (; cluster->dinfo.first_page_id % cluster->divisor != 0; cluster->dinfo.first_page_id++)
		{
			cluster->dinfo.page_count--;
		}
		cluster->total_cluster_count = cluster->dinfo.page_count / cluster->divisor;

	}
	else
	{
		cluster->divisor = cluster->dinfo.page_size / FS_CLUSTER_SIZE;
		cluster->total_cluster_count = cluster->dinfo.page_count * cluster->divisor;
	}
	cluster->bitmap_size = cluster->total_cluster_count / 8 + 1;
	os_map_init(&cluster->bitmaps, sizeof(uint32), sizeof(void *));
	cluster->is_update = 0;
	cluster->journal = NULL;
}

static uint32 do_cluster_alloc(os_cluster *cluster)
{
	uint32 i;
	uint32 id;
	if (cluster->cache_id != cluster->pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8))
	{
		cluster->cache_id = cluster->pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8);
		cluster->bitmap = bitmap_load(cluster->cache_id, cluster);
	}
	i = cluster->pcluster_manager->cur_index % (FS_CLUSTER_SIZE * 8) / 8;
	for (; i < FS_CLUSTER_SIZE; i++)
	{
		id = _bitmap_index[cluster->bitmap[i]];
		if (id != 8)
		{
			uint8 mark = 0x80;
			mark >>= id;
			cluster->bitmap[i] |= mark;
			id = 8 * i + id + FS_CLUSTER_SIZE * 8 * cluster->cache_id;
			cluster->pcluster_manager->used_cluster_count++;
			cluster->pcluster_manager->cur_index = id + 1;
			if (cluster->pcluster_manager->cur_index >= cluster->total_cluster_count)
			{
				cluster->pcluster_manager->cur_index = 0;
			}
			return id;
		}
	}
	cluster->pcluster_manager->cur_index = (cluster->cache_id + 1) * (FS_CLUSTER_SIZE * 8);
	if (cluster->pcluster_manager->cur_index >= cluster->total_cluster_count)
	{
		cluster->pcluster_manager->cur_index = 0;
	}
	return 0;
}

void cluster_manager_init(os_cluster *cluster)
{
	uint32 i;
	uint32 bitmap_cluster;
	cluster->pcluster_manager->cur_index = 0;
	cluster->pcluster_manager->used_cluster_count = 0;
	bitmap_cluster = cluster->bitmap_size / FS_CLUSTER_SIZE + 1;
	for (i = 0; i < bitmap_cluster; i++)
	{
		bitmap_init(i, cluster);
	}
	cluster->cache_id = 0;
	cluster->bitmap = bitmap_load(0, cluster);

	for (i = 0; i < 1 + 1 + bitmap_cluster; i++)
	{
		//前面簇分配出来预留
		do_cluster_alloc(cluster);
	}
	cluster_manager_flush(cluster);
}

uint32 cluster_alloc(os_cluster *cluster)
{
	uint32 id;
	while (cluster->pcluster_manager->used_cluster_count == cluster->total_cluster_count);
	cluster->is_update = 1;
	if (cluster->bitmap == NULL)
	{
		cluster->cache_id = cluster->pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8);
		cluster->bitmap = bitmap_load(cluster->cache_id, cluster);
	}
	id = do_cluster_alloc(cluster);
	for (; 0 == id;)
	{
		id = do_cluster_alloc(cluster);
	}
	return id;
}

void cluster_free(uint32 cluster_id, os_cluster *cluster)
{
	uint8 mark;
	uint32 i;
	uint32 j;
	if (cluster_id >= cluster->total_cluster_count)
	{
		return;
	}
	cluster->is_update = 1;
	if (cluster->bitmap == NULL)
	{
		cluster->cache_id = cluster_id / (FS_CLUSTER_SIZE * 8);
		cluster->bitmap = bitmap_load(cluster->cache_id, cluster);
	}
	if (cluster->cache_id != cluster_id / (FS_CLUSTER_SIZE * 8))
	{
		cluster->cache_id = cluster_id / (FS_CLUSTER_SIZE * 8);
		cluster->bitmap = bitmap_load(cluster->cache_id, cluster);
	}
	cluster_id %= (FS_CLUSTER_SIZE * 8);
	i = cluster_id >> 3;
	j = cluster_id % 8;
	mark = 0x80;
	mark >>= j;
	if ((cluster->bitmap[i] & mark) > 0)
	{
		mark = ~mark;
		cluster->bitmap[i] &= mark;
		cluster->pcluster_manager->used_cluster_count--;
	}
}

void cluster_read(uint32 cluster_id, uint8 *data, os_cluster *cluster)
{
	if (cluster->dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		uint32 i;
		for (i = 0; i < cluster->divisor; i++)
		{
			for (; os_disk_read(cluster->dinfo.first_page_id + cluster_id * cluster->divisor + i, data, cluster->dev_id) != 0; );
			data += FS_CLUSTER_SIZE / cluster->divisor;
		}
	}
	else
	{
		uint32 page_id = cluster->dinfo.first_page_id + cluster_id / cluster->divisor;
		uint32 i = cluster_id % cluster->divisor;
		uint8 *buff = (uint8 *)os_malloc(cluster->dinfo.page_size);
		for (; os_disk_read(page_id, buff, cluster->dev_id) != 0; );
		os_mem_cpy(data, &buff[i * FS_CLUSTER_SIZE], FS_CLUSTER_SIZE);
		os_free(buff);
	}
}

void cluster_write(uint32 cluster_id, uint8 *data, os_cluster *cluster)
{
	journal_write(cluster_id, (os_journal *)cluster->journal);
	if (cluster->dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		uint32 i;
		for (i = 0; i < cluster->divisor; i++)
		{
			for (; os_disk_write(cluster->dinfo.first_page_id + cluster_id * cluster->divisor + i, data, cluster->dev_id) != 0; );
			data += FS_CLUSTER_SIZE / cluster->divisor;
		}
	}
	else
	{
		uint32 page_id = cluster->dinfo.first_page_id + cluster_id / cluster->divisor;
		uint32 i = cluster_id % cluster->divisor;
		uint8 *buff = (uint8 *)os_malloc(cluster->dinfo.page_size);
		for (; os_disk_read(page_id, buff, cluster->dev_id) != 0; );
		os_mem_cpy(&buff[i * FS_CLUSTER_SIZE], data, FS_CLUSTER_SIZE);
		for (; os_disk_write(page_id, buff, cluster->dev_id) != 0; );
		os_free(buff);
	}
}

void set_journal(void *journal, os_cluster *cluster)
{
	cluster->journal = journal;
}

void cluster_flush(os_cluster *cluster)
{
	if (cluster->is_update)
	{
		cluster->is_update = 0;
		cluster_manager_flush(cluster);
		bitmaps_flush(cluster);
	}
}

void os_cluster_uninit(os_cluster *cluster)
{
	if (cluster->pcluster_manager != NULL)
	{
		os_free(cluster->pcluster_manager);
		cluster->pcluster_manager = NULL;
		if (cluster->bitmap != NULL)
		{
			os_free(cluster->bitmap);
			cluster->bitmap = NULL;
			cluster->cache_id = 0;
		}
	}
}

uint32 get_all_cluster_num(os_cluster *cluster)
{
	return cluster->total_cluster_count;
}

uint32 get_free_cluster_num(os_cluster *cluster)
{
	return cluster->total_cluster_count - cluster->pcluster_manager->used_cluster_count;
}
