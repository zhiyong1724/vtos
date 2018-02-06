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

static void bitmap_load(uint32 id, uint8 *data)
{
	cluster_read(id, data);
}

static void bitmap_flush(uint32 cluster, uint8 *data)
{
	cluster_write(cluster, data);
}

static void cluster_manager_flush(uint32 cluster)
{
	if (is_little_endian())
	{
		cluster_write(cluster, (uint8 *)_cluster_controler.pcluster_manager);
	}
	else
	{
		struct cluster_manager *temp = (struct cluster_manager *)malloc(FS_PAGE_SIZE);

		temp->cur_index = convert_endian(_cluster_controler.pcluster_manager->cur_index);
		temp->cluster_id = convert_endian(_cluster_controler.pcluster_manager->cluster_id);
		temp->used_cluster_count = convert_endian(_cluster_controler.pcluster_manager->used_cluster_count);
		cluster_write(cluster, (uint8 *)temp);
		free(temp);
	}
}

static void bitmap_init(uint32 cluster_id)
{
	uint32 ret;
	uint8 *temp = (uint8 *)malloc(FS_PAGE_SIZE);
	os_mem_set(temp, 0, FS_PAGE_SIZE);
	bitmap_flush(cluster_id, temp);
	free(temp);
}

void cluster_controler_init()
{
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

static uint32 do_cluster_alloc()
{
	uint32 i;
	uint32 id;
	if (_cluster_controler.cache_id != _cluster_controler.pcluster_manager->cur_index / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1)
	{
		bitmap_flush(_cluster_controler.cache_id, _cluster_controler.bitmap);
		_cluster_controler.cache_id = _cluster_controler.pcluster_manager->cur_index / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1;
		bitmap_load(_cluster_controler.cache_id, _cluster_controler.bitmap);
	}
	i = _cluster_controler.pcluster_manager->cur_index % (FS_PAGE_SIZE * 8) / 8;
	for (; i < FS_PAGE_SIZE; i++)
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

void cluster_manager_init()
{
	uint32 j;
	uint32 cluster_count;
	_cluster_controler.pcluster_manager->cur_index = 0;
	cluster_count = _cluster_controler.bitmap_size /  FS_PAGE_SIZE + 1 + 1;
	_cluster_controler.pcluster_manager->used_cluster_count = 0;
	_cluster_controler.pcluster_manager->cluster_id = 3;
	cluster_manager_flush(_cluster_controler.pcluster_manager->cluster_id);
	for (j = 1; j < cluster_count; j++)
	{
		bitmap_init(_cluster_controler.pcluster_manager->cluster_id + j);
	}
	if (j == cluster_count)
	{
		_cluster_controler.bitmap = (uint8 *)malloc(FS_PAGE_SIZE);
		_cluster_controler.cache_id = _cluster_controler.pcluster_manager->cluster_id + 1;
		bitmap_load(_cluster_controler.pcluster_manager->cluster_id + 1, _cluster_controler.bitmap);

		for (j = 0; j < 3 + cluster_count * 3; j++)
		{
			//前面簇分配出来预留
			do_cluster_alloc();
		}
	}
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
		bitmap_load(_cluster_controler.cache_id , _cluster_controler.bitmap);
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
		bitmap_load(_cluster_controler.cache_id , _cluster_controler.bitmap);
	}
	if (_cluster_controler.cache_id != cluster_id / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1)
	{
		bitmap_load(_cluster_controler.cache_id, _cluster_controler.bitmap);
		_cluster_controler.cache_id = cluster_id / (FS_PAGE_SIZE * 8)  + _cluster_controler.pcluster_manager->cluster_id + 1;
		bitmap_load(_cluster_controler.cache_id, _cluster_controler.bitmap);
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

void cluster_read(uint32 cluster_id, uint8 *data)
{
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

void cluster_manager_load(uint32 id)
{
	if (is_little_endian())
	{
		cluster_read(id, (uint8 *)_cluster_controler.pcluster_manager);
	}
	else
	{
		cluster_read(id, (uint8 *)_cluster_controler.pcluster_manager);
		_cluster_controler.pcluster_manager->cur_index = convert_endian(_cluster_controler.pcluster_manager->cur_index);
		_cluster_controler.pcluster_manager->cluster_id = convert_endian(_cluster_controler.pcluster_manager->cluster_id);
		_cluster_controler.pcluster_manager->used_cluster_count = convert_endian(_cluster_controler.pcluster_manager->used_cluster_count);
	}
}

void flush()
{
	cluster_manager_flush(_cluster_controler.pcluster_manager->cluster_id);
	if (_cluster_controler.bitmap != NULL)
	bitmap_flush(_cluster_controler.cache_id, _cluster_controler.bitmap);
}

void uninit()
{
	if (_cluster_controler.pcluster_manager != NULL)
	{
		flush();
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
