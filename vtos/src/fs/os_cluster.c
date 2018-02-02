#include "fs/os_cluster.h"
#include "lib/os_string.h"
#include "base/os_bitmap_index.h"
#include "vtos.h"
#include <stdlib.h>
static struct cluster_controler _cluster_controler;

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

static uint32 load(uint32 cluster, struct cluster_manager *data)
{
	uint32 ret = CLUSTER_NONE;
	if (is_little_endian())
	{
		ret = cluster_read(cluster, (uint8 *)data);
	}
	else
	{
		ret = cluster_read(cluster, (uint8 *)data);

		data->cur_index = convert_endian(data->cur_index);
		data->cluster_id = convert_endian(data->cluster_id);
		data->used_cluster_count = convert_endian(data->used_cluster_count);
	}
	return ret;
}

static uint32 bitmap_flush(uint32 cluster, uint8 *data)
{
	uint32 ret = CLUSTER_NONE;
	uint32 cluster_count = _cluster_controler.bitmap_size /  FS_PAGE_SIZE + 1 + 1;
	ret = cluster_write(cluster, data);
	if (CLUSTER_NONE == ret)
	{
		ret = cluster_write(cluster + cluster_count, data);
	}
	return ret;
}

static uint32 cluster_manager_flush(uint32 cluster)
{
	uint32 ret = CLUSTER_NONE;
	uint32 cluster_count = _cluster_controler.bitmap_size /  FS_PAGE_SIZE + 1 + 1;
	if (is_little_endian())
	{
		ret = cluster_write(cluster, (uint8 *)_cluster_controler.pcluster_manager);
		if (CLUSTER_NONE == ret)
		{
			ret = cluster_write(cluster + cluster_count, (uint8 *)_cluster_controler.pcluster_manager);
		}
	}
	else
	{
		struct cluster_manager *temp = (struct cluster_manager *)malloc(FS_PAGE_SIZE);

		temp->cur_index = convert_endian(_cluster_controler.pcluster_manager->cur_index);
		temp->cluster_id = convert_endian(_cluster_controler.pcluster_manager->cluster_id);
		temp->used_cluster_count = convert_endian(_cluster_controler.pcluster_manager->used_cluster_count);
		ret = cluster_write(cluster, (uint8 *)temp);
		if (CLUSTER_NONE == ret)
		{
			ret = cluster_write(cluster + cluster_count, (uint8 *)temp);
		}
		free(temp);
	}
	return ret;
}

static uint32 copy_bitmap(uint32 id1, uint32 id2)
{
	uint32 ret;
	uint32 cluster_count = _cluster_controler.bitmap_size /  FS_PAGE_SIZE + 1 + 1;
	if (id2 == _cluster_controler.cache_id + cluster_count)
	{
		ret = cluster_write(id1, _cluster_controler.bitmap);
		if (CLUSTER_NONE == ret)
		{
			ret = cluster_write(id1 + cluster_count, _cluster_controler.bitmap);
		}
	}
	else
	{
		uint8 *temp = (uint8 *)malloc(FS_PAGE_SIZE);
		ret = cluster_read(id2, temp);
		if (CLUSTER_NONE == ret)
		{
			ret = cluster_write(id1, temp);
			if (CLUSTER_NONE == ret)
			{
				ret = cluster_write(id1 + cluster_count, temp);
			}
		}
		free(temp);
	}
	return ret;
}

static uint32 handle_error()
{
	uint32 cluster_count = _cluster_controler.bitmap_size /  FS_PAGE_SIZE + 1 + 1;
	uint32 backup_bitmap = _cluster_controler.pcluster_manager->cluster_id + cluster_count;
	_cluster_controler.pcluster_manager->cluster_id += 2 * cluster_count;
	for (; _cluster_controler.pcluster_manager->cluster_id <= RETAIN_AREA_SIZE + cluster_count * (BACKUP_AREA_COUNT - 2); _cluster_controler.pcluster_manager->cluster_id += 2 * cluster_count)
	{
		if (cluster_manager_flush(_cluster_controler.pcluster_manager->cluster_id) == CLUSTER_NONE)
		{
			uint32 j;
			for (j = 1; j < cluster_count; j++)
			{
				uint32 ret = copy_bitmap(_cluster_controler.pcluster_manager->cluster_id + j, backup_bitmap + j);
				if (CLUSTER_READ_FAILED == ret)
				{
					return 1;
				}
				else if (CLUSTER_WRITE_FAILED == ret)
				{
					break;
				}
			}
			if (j == cluster_count)
			{
				_cluster_controler.on_cluster_manager_change(_cluster_controler.pcluster_manager->cluster_id);
				return 0;
			}
		}

	}
	return 1;
}

static uint32 bitmap_flush2(uint32 cluster, uint8 *data)
{
	uint32 ret = bitmap_flush(cluster, data);
	if (ret != CLUSTER_NONE)
	{
		if (handle_error() == CLUSTER_NONE)
		{
			ret = bitmap_flush(cluster, data);
		}
	}
	return ret;
}

static uint32 cluster_manager_flush2(uint32 cluster)
{
	uint32 ret = cluster_manager_flush(cluster);
	if (ret != CLUSTER_NONE)
	{
		if (handle_error() == CLUSTER_NONE)
		{
			ret = cluster_manager_flush(cluster);
		}
	}
	return ret;
}

static uint32 bitmap_init(uint32 cluster_id)
{
	uint32 ret;
	uint32 cluster_count = _cluster_controler.bitmap_size /  FS_PAGE_SIZE + 1 + 1;
	uint8 *temp = (uint8 *)malloc(FS_PAGE_SIZE);
	os_mem_set(temp, 0, FS_PAGE_SIZE);
	ret = bitmap_flush(cluster_id, temp);
	free(temp);
	return ret;
}

void cluster_controler_init()
{
	_cluster_controler.on_cluster_manager_change = NULL;
	_cluster_controler.pcluster_manager = (struct cluster_manager *)malloc(FS_PAGE_SIZE);
	_cluster_controler.bitmap = NULL;
	_cluster_controler.cache_id = 0;
	_cluster_controler.dinfo = os_get_disk_info();
	if (_cluster_controler.dinfo.page_size <= FS_PAGE_SIZE)
	{
		_cluster_controler.divisor = FS_PAGE_SIZE / _cluster_controler.dinfo.page_size;
		for (; _cluster_controler.dinfo.first_page_id % _cluster_controler.divisor != 0; _cluster_controler.dinfo.first_page_id++)
		{
			_cluster_controler.dinfo.page_count--;
		}
		_cluster_controler.total_cluster_count = _cluster_controler.dinfo.page_count / _cluster_controler.divisor;

	}
	else
	{
		_cluster_controler.divisor = _cluster_controler.dinfo.page_size / FS_PAGE_SIZE;
		_cluster_controler.total_cluster_count = _cluster_controler.dinfo.page_count * _cluster_controler.divisor;
	}
	_cluster_controler.bitmap_size = _cluster_controler.total_cluster_count / 8 + 1;
}

static uint32 bitmap_read(uint32 id, uint8 *data)
{
	uint32 ret = cluster_read(id, data);
	if (ret != CLUSTER_NONE)
	{
		_cluster_controler.cache_id = 0;
		ret = cluster_read(id + _cluster_controler.bitmap_size / FS_PAGE_SIZE + 1 + 1, data);
	}
	return ret;
}

static uint32 do_cluster_alloc()
{
	uint32 i;
	uint32 id;
	if (_cluster_controler.cache_id != _cluster_controler.pcluster_manager->cur_index / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1)
	{
		if (bitmap_flush2(_cluster_controler.cache_id, _cluster_controler.bitmap) != CLUSTER_NONE)
		{
			return 0;
		}
		_cluster_controler.cache_id = _cluster_controler.pcluster_manager->cur_index / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1;
		if (bitmap_read(_cluster_controler.cache_id, _cluster_controler.bitmap) != CLUSTER_NONE)
		{
			return 0;
		}
	}
	for (i = 0; i < FS_PAGE_SIZE; i++)
	{
		id = _bitmap_index[_cluster_controler.bitmap[i]];
		if (id != 8)
		{
			uint8 mark = 0x80;
			mark >>= id;
			_cluster_controler.bitmap[i] |= mark;
			id = 8 * i + id + FS_PAGE_SIZE * 8 * (_cluster_controler.cache_id - _cluster_controler.pcluster_manager->cluster_id - 1);
			_cluster_controler.pcluster_manager->used_cluster_count++;
			_cluster_controler.pcluster_manager->cur_index = id + 1;
			if (_cluster_controler.pcluster_manager->cur_index == _cluster_controler.total_cluster_count)
			{
				_cluster_controler.pcluster_manager->cur_index = 0;
			}
			return id;
		}
	}
	_cluster_controler.pcluster_manager->cur_index = FS_PAGE_SIZE * 8 * (_cluster_controler.cache_id - _cluster_controler.pcluster_manager->cluster_id - 1 + 1);
	if (_cluster_controler.pcluster_manager->cur_index == _cluster_controler.total_cluster_count)
	{
		_cluster_controler.pcluster_manager->cur_index = 0;
	}
	return 0;
}

uint32 cluster_manager_init()
{
	uint32 cluster_count;
	_cluster_controler.pcluster_manager->cur_index = 0;
	cluster_count = _cluster_controler.bitmap_size /  FS_PAGE_SIZE + 1 + 1;
	_cluster_controler.pcluster_manager->used_cluster_count = 0;
	for (_cluster_controler.pcluster_manager->cluster_id = RETAIN_AREA_SIZE; _cluster_controler.pcluster_manager->cluster_id <= RETAIN_AREA_SIZE + cluster_count * (BACKUP_AREA_COUNT - 2); _cluster_controler.pcluster_manager->cluster_id += 2 * cluster_count)
	{
		if (cluster_manager_flush(_cluster_controler.pcluster_manager->cluster_id) == CLUSTER_NONE)
		{
			uint32 j;
			for (j = 1; j < cluster_count; j++)
			{
				if (bitmap_init(_cluster_controler.pcluster_manager->cluster_id + j) != CLUSTER_NONE)
				{
					break;
				}
			}
			if (j == cluster_count)
			{
				_cluster_controler.bitmap = (uint8 *)malloc(FS_PAGE_SIZE);
				_cluster_controler.cache_id = _cluster_controler.pcluster_manager->cluster_id + 1;
				if (bitmap_read(_cluster_controler.pcluster_manager->cluster_id + 1, _cluster_controler.bitmap) != CLUSTER_NONE)
				{
					free(_cluster_controler.bitmap);
					break;
				}
				
				for (j = 0; j < RETAIN_AREA_SIZE + cluster_count * BACKUP_AREA_COUNT; j++)
				{
					//前面簇分配出来预留
					do_cluster_alloc();
				}
				return _cluster_controler.pcluster_manager->cluster_id;
			}
		}
		
	}
	return 0;
}

uint32 cluster_alloc()
{
	uint32 id;
	if (_cluster_controler.pcluster_manager->used_cluster_count == _cluster_controler.total_cluster_count)
	{
		return 0;
	}
	if (_cluster_controler.bitmap == NULL)
	{
		_cluster_controler.bitmap = (uint8 *)malloc(FS_PAGE_SIZE);
		_cluster_controler.cache_id = _cluster_controler.pcluster_manager->cur_index / (FS_PAGE_SIZE * 8) + _cluster_controler.pcluster_manager->cluster_id + 1;
		if (bitmap_read(_cluster_controler.cache_id , _cluster_controler.bitmap) != CLUSTER_NONE)
		{
			return 0;
		}
	}
	id = do_cluster_alloc();
	for(; 0 == id; )
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
	if (cluster_id >= _cluster_controler.total_cluster_count)
	{
		return;
	}
	if (_cluster_controler.bitmap == NULL)
	{
		_cluster_controler.bitmap = (uint8 *)malloc(FS_PAGE_SIZE);
		_cluster_controler.cache_id = cluster_id / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1;
		if (bitmap_read(_cluster_controler.cache_id , _cluster_controler.bitmap) != CLUSTER_NONE)
		{
			return;
		}
	}
	if (_cluster_controler.cache_id != cluster_id / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1)
	{
		if (bitmap_flush2(_cluster_controler.cache_id, _cluster_controler.bitmap) != CLUSTER_NONE)
		{
			return;
		}
		_cluster_controler.cache_id = cluster_id / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1;
		if (bitmap_read(_cluster_controler.cache_id, _cluster_controler.bitmap) != CLUSTER_NONE)
		{
			return;
		}
	}
	cluster_id %= (FS_PAGE_SIZE * 8);
	i = cluster_id >> 3;
	j = cluster_id % 8;
	mark = 0x80;
	mark >>= j;
	mark = ~mark;
	_cluster_controler.bitmap[i] &= mark;
	_cluster_controler.pcluster_manager->used_cluster_count--;
}

uint32 cluster_read(uint32 cluster_id, uint8 *data)
{
	uint32 ret = CLUSTER_READ_FAILED;
	if (_cluster_controler.dinfo.page_size <= FS_PAGE_SIZE)
	{
		uint32 i;
		for (i = 0; i < _cluster_controler.divisor && os_disk_read(_cluster_controler.dinfo.first_page_id + cluster_id * _cluster_controler.divisor + i, data) == 0; i++, data += _cluster_controler.divisor);
		if (i == _cluster_controler.divisor)
		{
			ret = CLUSTER_NONE;
		}
	}
	else
	{
		uint32 page_id = _cluster_controler.dinfo.first_page_id + cluster_id / _cluster_controler.divisor;
		uint32 i = cluster_id % _cluster_controler.divisor;
		uint8 *buff = (uint8 *)malloc(_cluster_controler.dinfo.page_size);
		if (0 == os_disk_read(page_id, buff))
		{
			os_mem_cpy(data, &buff[i * FS_PAGE_SIZE], FS_PAGE_SIZE);
			ret = CLUSTER_NONE;
		}
		free(buff);
	}
	return ret;
}

uint32 cluster_write(uint32 cluster_id, uint8 *data)
{
	uint32 ret = CLUSTER_WRITE_FAILED;
	if (_cluster_controler.dinfo.page_size <= FS_PAGE_SIZE)
	{
		uint32 i;
		for (i = 0; i < _cluster_controler.divisor && os_disk_write(_cluster_controler.dinfo.first_page_id + cluster_id * _cluster_controler.divisor + i, data) == 0; i++, data += _cluster_controler.divisor);
		if (i == _cluster_controler.divisor)
		{
			ret = CLUSTER_NONE;
		}
	}
	else
	{
		uint32 page_id = _cluster_controler.dinfo.first_page_id + cluster_id / _cluster_controler.divisor;
		uint32 i = cluster_id % _cluster_controler.divisor;
		uint8 *buff = (uint8 *)malloc(_cluster_controler.dinfo.page_size);
		if (0 == os_disk_read(page_id, buff))
		{
			os_mem_cpy(&buff[i * FS_PAGE_SIZE], data, FS_PAGE_SIZE);
			if (0 == os_disk_write(page_id, buff))
			{
				ret = CLUSTER_NONE;
			}
		}
		free(buff);
	}
	return ret;
}

uint32 cluster_manager_load(uint32 id)
{
	uint32 ret = 0;
	if (is_little_endian())
	{
		ret = cluster_read(id, (uint8 *)_cluster_controler.pcluster_manager);
		if (ret != CLUSTER_NONE)
		{
			id += _cluster_controler.bitmap_size / FS_PAGE_SIZE + 1 + 1;
			ret = cluster_read(id, (uint8 *)_cluster_controler.pcluster_manager);
		}
	}
	else
	{
		ret = cluster_read(id, (uint8 *)_cluster_controler.pcluster_manager);
		if (ret != CLUSTER_NONE)
		{
			id += _cluster_controler.bitmap_size / FS_PAGE_SIZE + 1 + 1;
			ret = cluster_read(id, (uint8 *)_cluster_controler.pcluster_manager);
		}
		if (CLUSTER_NONE == ret)
		{
			_cluster_controler.pcluster_manager->cur_index = convert_endian(_cluster_controler.pcluster_manager->cur_index);
			_cluster_controler.pcluster_manager->cluster_id = convert_endian(_cluster_controler.pcluster_manager->cluster_id);
			_cluster_controler.pcluster_manager->used_cluster_count = convert_endian(_cluster_controler.pcluster_manager->used_cluster_count);
		}
	}
	
 	return ret;
}

uint32 flush2()
{
	uint32 ret;
	ret = cluster_manager_flush2(_cluster_controler.pcluster_manager->cluster_id);
	if (CLUSTER_NONE == ret && _cluster_controler.bitmap != NULL)
	{
		ret = bitmap_flush2(_cluster_controler.cache_id, _cluster_controler.bitmap);
	}
	return ret;
}

void uninit()
{
	if (_cluster_controler.pcluster_manager != NULL)
	{
		flush2();
		free(_cluster_controler.pcluster_manager);
		_cluster_controler.pcluster_manager = NULL;
		if (_cluster_controler.bitmap != NULL)
		{
			free(_cluster_controler.bitmap);
			_cluster_controler.bitmap = NULL;
			_cluster_controler.cache_id = 0;
		}
	}
}

void register_manager_callback(void(*on_cluster_manager_change)(uint32 cluster_id))
{
	_cluster_controler.on_cluster_manager_change = on_cluster_manager_change;
}