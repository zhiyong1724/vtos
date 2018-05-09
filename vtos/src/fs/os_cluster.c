#include "fs/os_cluster.h"
#include "base/os_string.h"
#include "base/os_bitmap_index.h"
#include "os_journal.h"
#include "vtos.h"
static struct os_cluster _os_cluster;
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

static uint8 *bitmap_load(uint32 id)
{
	uint8 *data = NULL;
	os_map_iterator *itr = os_map_find(&_os_cluster.bitmaps, &id);
	if (itr != NULL)
	{
		uint8 **pp = (uint8 **)os_map_second(&_os_cluster.bitmaps, itr);
		data = *pp;
	}
	else
	{
		data = (uint8 *)os_kmalloc(FS_CLUSTER_SIZE);
		cluster_read(FIRST_CLUSTER_MANAGER_ID + 1 + id, data);
		os_map_insert(&_os_cluster.bitmaps, &id, &data);
	}
	return data;
}

static void bitmap_flush(uint32 id, uint8 *data)
{
	cluster_write(FIRST_CLUSTER_MANAGER_ID + 1 + id, data);
}

static void bitmaps_flush()
{
	os_map_iterator *itr = os_map_begin(&_os_cluster.bitmaps);
	for (; itr != NULL; (itr = os_map_next(&_os_cluster.bitmaps, itr)))
	{
		uint32 *key = (uint32 *)os_map_first(itr);
		uint8 **pp = (uint8 **)os_map_second(&_os_cluster.bitmaps, itr);
		bitmap_flush(*key, *pp);
		if (*key != _os_cluster.cache_id)
		{
			os_kfree(*pp);
		}
	}
	os_map_clear(&_os_cluster.bitmaps);
	bitmap_flush(_os_cluster.cache_id, _os_cluster.bitmap);
}

static void cluster_manager_flush()
{
	if (is_little_endian())
	{
		cluster_write(FIRST_CLUSTER_MANAGER_ID, (uint8 *)_os_cluster.pcluster_manager);
	}
	else
	{
		struct cluster_manager *temp = (struct cluster_manager *)os_kmalloc(FS_CLUSTER_SIZE);
		temp->cur_index = convert_endian(_os_cluster.pcluster_manager->cur_index);
		temp->used_cluster_count = convert_endian(_os_cluster.pcluster_manager->used_cluster_count);
		cluster_write(FIRST_CLUSTER_MANAGER_ID, (uint8 *)temp);
		os_kfree(temp);
	}
}

void cluster_manager_load()
{
	if (is_little_endian())
	{
		cluster_read(FIRST_CLUSTER_MANAGER_ID, (uint8 *)_os_cluster.pcluster_manager);
	}
	else
	{
		cluster_read(FIRST_CLUSTER_MANAGER_ID, (uint8 *)_os_cluster.pcluster_manager);
		_os_cluster.pcluster_manager->cur_index = convert_endian(_os_cluster.pcluster_manager->cur_index);
		if (_os_cluster.pcluster_manager->cur_index >= _os_cluster.total_cluster_count)
		{
			_os_cluster.pcluster_manager->cur_index = 0;
		}
		_os_cluster.pcluster_manager->used_cluster_count = convert_endian(_os_cluster.pcluster_manager->used_cluster_count);
	}
}

static void bitmap_init(uint32 id)
{
	uint8 *temp = (uint8 *)os_kmalloc(FS_CLUSTER_SIZE);
	os_mem_set(temp, 0, FS_CLUSTER_SIZE);
	bitmap_flush(id, temp);
	os_kfree(temp);
}

void os_cluster_init()
{
	_os_cluster.pcluster_manager = (struct cluster_manager *)os_kmalloc(FS_CLUSTER_SIZE);
	_os_cluster.bitmap = NULL;
	_os_cluster.cache_id = 0;
	_os_cluster.dinfo = os_get_disk_info();
	if (_os_cluster.dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		_os_cluster.divisor = FS_CLUSTER_SIZE / _os_cluster.dinfo.page_size;
		for (; _os_cluster.dinfo.first_page_id % _os_cluster.divisor != 0; _os_cluster.dinfo.first_page_id++)
		{
			_os_cluster.dinfo.page_count--;
		}
		_os_cluster.total_cluster_count = _os_cluster.dinfo.page_count / _os_cluster.divisor;

	}
	else
	{
		_os_cluster.divisor = _os_cluster.dinfo.page_size / FS_CLUSTER_SIZE;
		_os_cluster.total_cluster_count = _os_cluster.dinfo.page_count * _os_cluster.divisor;
	}
	_os_cluster.bitmap_size = _os_cluster.total_cluster_count / 8 + 1;
	os_map_init(&_os_cluster.bitmaps, sizeof(uint32), sizeof(void *));
	_os_cluster.is_update = 0;
}

static uint32 do_cluster_alloc()
{
	uint32 i;
	uint32 id;
	if (_os_cluster.cache_id != _os_cluster.pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8))
	{
		_os_cluster.cache_id = _os_cluster.pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8);
		_os_cluster.bitmap = bitmap_load(_os_cluster.cache_id);
	}
	i = _os_cluster.pcluster_manager->cur_index % (FS_CLUSTER_SIZE * 8) / 8;
	for (; i < FS_CLUSTER_SIZE; i++)
	{
		id = _bitmap_index[_os_cluster.bitmap[i]];
		if (id != 8)
		{
			uint8 mark = 0x80;
			mark >>= id;
			_os_cluster.bitmap[i] |= mark;
			id = 8 * i + id + FS_CLUSTER_SIZE * 8 * _os_cluster.cache_id;
			_os_cluster.pcluster_manager->used_cluster_count++;
			_os_cluster.pcluster_manager->cur_index = id + 1;
			if (_os_cluster.pcluster_manager->cur_index >= _os_cluster.total_cluster_count)
			{
				_os_cluster.pcluster_manager->cur_index = 0;
			}
			return id;
		}
	}
	_os_cluster.pcluster_manager->cur_index = (_os_cluster.cache_id + 1) * (FS_CLUSTER_SIZE * 8);
	if (_os_cluster.pcluster_manager->cur_index >= _os_cluster.total_cluster_count)
	{
		_os_cluster.pcluster_manager->cur_index = 0;
	}
	return 0;
}

void cluster_manager_init()
{
	uint32 i;
	uint32 bitmap_cluster;
	_os_cluster.pcluster_manager->cur_index = 0;
	_os_cluster.pcluster_manager->used_cluster_count = 0;
	bitmap_cluster = _os_cluster.bitmap_size / FS_CLUSTER_SIZE + 1;
	for (i = 0; i < bitmap_cluster; i++)
	{
		bitmap_init(i);
	}
	_os_cluster.cache_id = 0;
	_os_cluster.bitmap = bitmap_load(0);

	for (i = 0; i < 1 + 1 + bitmap_cluster; i++)
	{
		//前面簇分配出来预留
		do_cluster_alloc();
	}
	cluster_manager_flush();
}

uint32 cluster_alloc()
{
	uint32 id;
	if (_os_cluster.pcluster_manager->used_cluster_count == _os_cluster.total_cluster_count)
	{
		return 0;
	}
	_os_cluster.is_update = 1;
	if (_os_cluster.bitmap == NULL)
	{
		_os_cluster.cache_id = _os_cluster.pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8);
		_os_cluster.bitmap = bitmap_load(_os_cluster.cache_id);
	}
	id = do_cluster_alloc();
	for (; 0 == id;)
	{
		id = do_cluster_alloc();
	}
	return id;
}

void cluster_free(uint32 cluster_id)
{
	uint8 mark;
	uint32 i;
	uint32 j;
	if (cluster_id >= _os_cluster.total_cluster_count)
	{
		return;
	}
	_os_cluster.is_update = 1;
	if (_os_cluster.bitmap == NULL)
	{
		_os_cluster.cache_id = cluster_id / (FS_CLUSTER_SIZE * 8);
		_os_cluster.bitmap = bitmap_load(_os_cluster.cache_id);
	}
	if (_os_cluster.cache_id != cluster_id / (FS_CLUSTER_SIZE * 8))
	{
		_os_cluster.cache_id = cluster_id / (FS_CLUSTER_SIZE * 8);
		_os_cluster.bitmap = bitmap_load(_os_cluster.cache_id);
	}
	cluster_id %= (FS_CLUSTER_SIZE * 8);
	i = cluster_id >> 3;
	j = cluster_id % 8;
	mark = 0x80;
	mark >>= j;
	if ((_os_cluster.bitmap[i] & mark) > 0)
	{
		mark = ~mark;
		_os_cluster.bitmap[i] &= mark;
		_os_cluster.pcluster_manager->used_cluster_count--;
	}
}

void cluster_read(uint32 cluster_id, uint8 *data)
{
	if (_os_cluster.dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		uint32 i;
		for (i = 0; i < _os_cluster.divisor; i++)
		{
			for (; os_disk_read(_os_cluster.dinfo.first_page_id + cluster_id * _os_cluster.divisor + i, data) != 0; );
			data += FS_CLUSTER_SIZE / _os_cluster.divisor;
		}
	}
	else
	{
		uint32 page_id = _os_cluster.dinfo.first_page_id + cluster_id / _os_cluster.divisor;
		uint32 i = cluster_id % _os_cluster.divisor;
		uint8 *buff = (uint8 *)os_kmalloc(_os_cluster.dinfo.page_size);
		for (; os_disk_read(page_id, buff) != 0; );
		os_mem_cpy(data, &buff[i * FS_CLUSTER_SIZE], FS_CLUSTER_SIZE);
		os_kfree(buff);
	}
}

void cluster_write(uint32 cluster_id, uint8 *data)
{
	journal_write(cluster_id);
	if (_os_cluster.dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		uint32 i;
		for (i = 0; i < _os_cluster.divisor; i++)
		{
			for (; os_disk_write(_os_cluster.dinfo.first_page_id + cluster_id * _os_cluster.divisor + i, data) != 0; );
			data += FS_CLUSTER_SIZE / _os_cluster.divisor;
		}
	}
	else
	{
		uint32 page_id = _os_cluster.dinfo.first_page_id + cluster_id / _os_cluster.divisor;
		uint32 i = cluster_id % _os_cluster.divisor;
		uint8 *buff = (uint8 *)os_kmalloc(_os_cluster.dinfo.page_size);
		for (; os_disk_read(page_id, buff) != 0; );
		os_mem_cpy(&buff[i * FS_CLUSTER_SIZE], data, FS_CLUSTER_SIZE);
		for (; os_disk_write(page_id, buff) != 0; );
		os_kfree(buff);
	}
}

void cluster_flush()
{
	if (_os_cluster.is_update)
	{
		_os_cluster.is_update = 0;
		cluster_manager_flush();
		bitmaps_flush();
	}
}

void uninit()
{
	if (_os_cluster.pcluster_manager != NULL)
	{
		os_kfree(_os_cluster.pcluster_manager);
		_os_cluster.pcluster_manager = NULL;
		if (_os_cluster.bitmap != NULL)
		{
			os_kfree(_os_cluster.bitmap);
			_os_cluster.bitmap = NULL;
			_os_cluster.cache_id = 0;
		}
	}
}

uint32 get_all_cluster_num()
{
	return _os_cluster.total_cluster_count;
}

uint32 get_free_cluster_num()
{
	return _os_cluster.total_cluster_count - _os_cluster.pcluster_manager->used_cluster_count;
}
