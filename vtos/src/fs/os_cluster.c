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
	cluster_read(FIRST_CLUSTER_MANAGER_ID + 1 + id, data);
}

static void bitmap_flush(uint32 id, uint8 *data)
{
	cluster_write(FIRST_CLUSTER_MANAGER_ID + 1 + id, data);
}

static void cluster_manager_flush()
{
	if (is_little_endian())
	{
		cluster_write(FIRST_CLUSTER_MANAGER_ID, (uint8 *)_cluster_controler.pcluster_manager);
	}
	else
	{
		struct cluster_manager *temp = (struct cluster_manager *)malloc(FS_CLUSTER_SIZE);

		temp->cur_index = convert_endian(_cluster_controler.pcluster_manager->cur_index);
		temp->used_cluster_count = convert_endian(_cluster_controler.pcluster_manager->used_cluster_count);
		cluster_write(FIRST_CLUSTER_MANAGER_ID, (uint8 *)temp);
		free(temp);
	}
}

void cluster_manager_load()
{
	if (is_little_endian())
	{
		cluster_read(FIRST_CLUSTER_MANAGER_ID, (uint8 *)_cluster_controler.pcluster_manager);
	}
	else
	{
		cluster_read(FIRST_CLUSTER_MANAGER_ID, (uint8 *)_cluster_controler.pcluster_manager);
		_cluster_controler.pcluster_manager->cur_index = convert_endian(_cluster_controler.pcluster_manager->cur_index);
		_cluster_controler.pcluster_manager->used_cluster_count = convert_endian(_cluster_controler.pcluster_manager->used_cluster_count);
	}
}

static void bitmap_init(uint32 id)
{
	uint8 *temp = (uint8 *)malloc(FS_CLUSTER_SIZE);
	os_mem_set(temp, 0, FS_CLUSTER_SIZE);
	bitmap_flush(id, temp);
	free(temp);
}

void cluster_controler_init()
{
	_cluster_controler.pcluster_manager = (struct cluster_manager *)malloc(FS_CLUSTER_SIZE);
	_cluster_controler.bitmap = NULL;
	_cluster_controler.cache_id = 0;
	_cluster_controler.dinfo = os_get_disk_info();
	if (_cluster_controler.dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		_cluster_controler.divisor = FS_CLUSTER_SIZE / _cluster_controler.dinfo.page_size;
		for (; _cluster_controler.dinfo.first_page_id % _cluster_controler.divisor != 0; _cluster_controler.dinfo.first_page_id++)
		{
			_cluster_controler.dinfo.page_count--;
		}
		_cluster_controler.total_cluster_count = _cluster_controler.dinfo.page_count / _cluster_controler.divisor;

	}
	else
	{
		_cluster_controler.divisor = _cluster_controler.dinfo.page_size / FS_CLUSTER_SIZE;
		_cluster_controler.total_cluster_count = _cluster_controler.dinfo.page_count * _cluster_controler.divisor;
	}
	_cluster_controler.bitmap_size = _cluster_controler.total_cluster_count / 8 + 1;
}

static uint32 do_cluster_alloc()
{
	uint32 i;
	uint32 id;
	if (_cluster_controler.cache_id != _cluster_controler.pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8))
	{
		bitmap_flush(_cluster_controler.cache_id, _cluster_controler.bitmap);
		_cluster_controler.cache_id = _cluster_controler.pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8);
		bitmap_load(_cluster_controler.cache_id, _cluster_controler.bitmap);
	}
	i = _cluster_controler.pcluster_manager->cur_index % (FS_CLUSTER_SIZE * 8) / 8;
	for (; i < FS_CLUSTER_SIZE; i++)
	{
		id = _bitmap_index[_cluster_controler.bitmap[i]];
		if (id != 8)
		{
			uint8 mark = 0x80;
			mark >>= id;
			_cluster_controler.bitmap[i] |= mark;
			id = 8 * i + id + FS_CLUSTER_SIZE * 8 * _cluster_controler.cache_id;
			_cluster_controler.pcluster_manager->used_cluster_count++;
			_cluster_controler.pcluster_manager->cur_index = id + 1;
			if (_cluster_controler.pcluster_manager->cur_index >= _cluster_controler.total_cluster_count)
			{
				_cluster_controler.pcluster_manager->cur_index = 0;
			}
			return id;
		}
	}
	_cluster_controler.pcluster_manager->cur_index = (_cluster_controler.cache_id + 1) * (FS_CLUSTER_SIZE * 8);
	if (_cluster_controler.pcluster_manager->cur_index >= _cluster_controler.total_cluster_count)
	{
		_cluster_controler.pcluster_manager->cur_index = 0;
	}
	return 0;
}

void cluster_manager_init()
{
	uint32 i;
	uint32 bitmap_cluster;
	_cluster_controler.pcluster_manager->cur_index = 0;
	_cluster_controler.pcluster_manager->used_cluster_count = 0;
	bitmap_cluster = _cluster_controler.bitmap_size / FS_CLUSTER_SIZE + 1;
	for (i = 0; i < bitmap_cluster; i++)
	{
		bitmap_init(i);
	}
	_cluster_controler.bitmap = (uint8 *)malloc(FS_CLUSTER_SIZE);
	_cluster_controler.cache_id = 0;
	bitmap_load(0, _cluster_controler.bitmap);

	for (i = 0; i < SUPER_CLUSTER_ID + 3 + 1 + bitmap_cluster; i++)
	{
		//前面簇分配出来预留
		do_cluster_alloc();
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
		_cluster_controler.bitmap = (uint8 *)malloc(FS_CLUSTER_SIZE);
		_cluster_controler.cache_id = _cluster_controler.pcluster_manager->cur_index / (FS_CLUSTER_SIZE * 8);
		bitmap_load(_cluster_controler.cache_id, _cluster_controler.bitmap);
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
	if (cluster_id >= _cluster_controler.total_cluster_count)
	{
		return;
	}
	if (_cluster_controler.bitmap == NULL)
	{
		_cluster_controler.bitmap = (uint8 *)malloc(FS_CLUSTER_SIZE);
		_cluster_controler.cache_id = cluster_id / (FS_CLUSTER_SIZE * 8);
		bitmap_load(_cluster_controler.cache_id, _cluster_controler.bitmap);
	}
	if (_cluster_controler.cache_id != cluster_id / (FS_CLUSTER_SIZE * 8))
	{
		bitmap_flush(_cluster_controler.cache_id, _cluster_controler.bitmap);
		_cluster_controler.cache_id = cluster_id / (FS_CLUSTER_SIZE * 8);
		bitmap_load(_cluster_controler.cache_id, _cluster_controler.bitmap);
	}
	cluster_id %= (FS_CLUSTER_SIZE * 8);
	i = cluster_id >> 3;
	j = cluster_id % 8;
	mark = 0x80;
	mark >>= j;
	if ((_cluster_controler.bitmap[i] & mark) > 0)
	{
		mark = ~mark;
		_cluster_controler.bitmap[i] &= mark;
		_cluster_controler.pcluster_manager->used_cluster_count--;
	}
}

void cluster_read(uint32 cluster_id, uint8 *data)
{
	if (_cluster_controler.dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		uint32 i;
		for (i = 0; i < _cluster_controler.divisor; i++)
		{
			while (os_disk_read(_cluster_controler.dinfo.first_page_id + cluster_id * _cluster_controler.divisor + i, data) != 0);
			data += FS_CLUSTER_SIZE / _cluster_controler.divisor;
		}
	}
	else
	{
		uint32 page_id = _cluster_controler.dinfo.first_page_id + cluster_id / _cluster_controler.divisor;
		uint32 i = cluster_id % _cluster_controler.divisor;
		uint8 *buff = (uint8 *)malloc(_cluster_controler.dinfo.page_size);
		while (os_disk_read(page_id, buff) != 0);
		os_mem_cpy(data, &buff[i * FS_CLUSTER_SIZE], FS_CLUSTER_SIZE);
		free(buff);
	}
}

uint32 cluster_read_return(uint32 cluster_id, uint8 *data)
{
	if (_cluster_controler.dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		uint32 i;
		for (i = 0; i < _cluster_controler.divisor; i++)
		{
			if (os_disk_read(_cluster_controler.dinfo.first_page_id + cluster_id * _cluster_controler.divisor + i, data) != 0)
			{
				return 1;
			}
			data += FS_CLUSTER_SIZE / _cluster_controler.divisor;
		}
	}
	else
	{
		uint32 page_id = _cluster_controler.dinfo.first_page_id + cluster_id / _cluster_controler.divisor;
		uint32 i = cluster_id % _cluster_controler.divisor;
		uint8 *buff = (uint8 *)malloc(_cluster_controler.dinfo.page_size);
		if (os_disk_read(page_id, buff) != 0)
		{
			free(buff);
			return 1;
		}
		os_mem_cpy(data, &buff[i * FS_CLUSTER_SIZE], FS_CLUSTER_SIZE);
		free(buff);
	}
	return 0;
}

void cluster_write(uint32 cluster_id, uint8 *data)
{
	if (_cluster_controler.dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		uint32 i;
		for (i = 0; i < _cluster_controler.divisor; i++)
		{
			while (os_disk_write(_cluster_controler.dinfo.first_page_id + cluster_id * _cluster_controler.divisor + i, data) != 0);
			data += FS_CLUSTER_SIZE / _cluster_controler.divisor;
		}
	}
	else
	{
		uint32 page_id = _cluster_controler.dinfo.first_page_id + cluster_id / _cluster_controler.divisor;
		uint32 i = cluster_id % _cluster_controler.divisor;
		uint8 *buff = (uint8 *)malloc(_cluster_controler.dinfo.page_size);
		while (os_disk_read(page_id, buff) != 0);
		os_mem_cpy(&buff[i * FS_CLUSTER_SIZE], data, FS_CLUSTER_SIZE);
		while (os_disk_write(page_id, buff) != 0);
		free(buff);
	}
}

uint32 cluster_write_return(uint32 cluster_id, uint8 *data)
{
	if (_cluster_controler.dinfo.page_size <= FS_CLUSTER_SIZE)
	{
		uint32 i;
		for (i = 0; i < _cluster_controler.divisor; i++)
		{
			if (os_disk_write(_cluster_controler.dinfo.first_page_id + cluster_id * _cluster_controler.divisor + i, data) != 0)
			{
				return 1;
			}
			data += FS_CLUSTER_SIZE / _cluster_controler.divisor;
		}
	}
	else
	{
		uint32 page_id = _cluster_controler.dinfo.first_page_id + cluster_id / _cluster_controler.divisor;
		uint32 i = cluster_id % _cluster_controler.divisor;
		uint8 *buff = (uint8 *)malloc(_cluster_controler.dinfo.page_size);
		if (os_disk_read(page_id, buff) != 0)
		{
			free(buff);
			return 1;
		}
		os_mem_cpy(&buff[i * FS_CLUSTER_SIZE], data, FS_CLUSTER_SIZE);
		if (os_disk_write(page_id, buff) != 0)
		{
			free(buff);
			return 1;
		}
		free(buff);
	}
	return 0;
}

void flush()
{
	cluster_manager_flush();
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

uint32 get_all_cluster_num()
{
	return _cluster_controler.total_cluster_count;
}

uint32 get_free_cluster_num()
{
	return _cluster_controler.total_cluster_count - _cluster_controler.pcluster_manager->used_cluster_count;
}
