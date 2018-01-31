#include "fs/os_fs.h"
#include "lib/os_string.h"
#include "fs/os_dentry.h"
#include <stdio.h>
#include <vtos.h>
#include <stdlib.h>
static super_cluster *_super = NULL;
static fnode *_root = NULL;
static fnode *_backup = NULL;
static uint32 get_file_name(char *dest, char *path, uint32 *index)
{
	if ('/' == *path)
	{
		(*index)++;
		for (; path[*index] != '\0'; (*index)++)
		{
			if (path[*index] != '/')
			{
				break;
			}
		}
		for (; path[*index] != '\0'; (*index)++)
		{
			if (path[*index] == '/')
			{
				break;
			}
			else
			{
				*dest = path[*index];
				dest++;
			}
		}
		dest++;
		*dest = '\0';
	}
	return 0;
}

static void super_cluster_init(super_cluster *super)
{
	super->flag = 0xaa55a55a;
	super->cluster_id = 0;
	os_str_cpy(super->name, "emfs", 16);
	super->bitmap_id = 0;
	super->root_id = 0;
	super->backup_id = 0;
}

static void file_info_init(file_info *info)
{
	info->cluster_id = 0;
	info->size = 0;
	info->cluster_count = 0;
	info->create_time = 0;
	info->modif_time = 0;
	info->creator = 0;
	info->modifier = 0;
	info->limits = 0;
	info->backup_id = 0;
	info->name[0] = '\0';
}

static uint32 super_cluster_flush(uint32 cluster)
{
	uint32 ret = CLUSTER_NONE;
	if (is_little_endian())
	{
		ret = cluster_write(cluster, (uint8 *)_super);
		if (CLUSTER_NONE == ret)
		{
			ret = cluster_write(cluster + 1, (uint8 *)_super);
		}
	}
	else
	{
		super_cluster *temp = (super_cluster *)malloc(FS_PAGE_SIZE);
		os_mem_cpy(temp, _super, FS_PAGE_SIZE);

		temp->flag = convert_endian(_super->flag);
		temp->cluster_id = convert_endian(_super->cluster_id);
		temp->bitmap_id = convert_endian(_super->bitmap_id);
		temp->root_id = convert_endian(_super->root_id);
		temp->backup_id = convert_endian(_super->backup_id);
		ret = cluster_write(cluster, (uint8 *)temp);
		if (CLUSTER_NONE == ret)
		{
			ret = cluster_write(cluster + 1, (uint8 *)temp);
		}
		free(temp);
	}
	return ret;
}

static uint32 handle_error()
{
	uint32 ret = 1;
	for (_super->cluster_id += 2; _super->cluster_id < RETAIN_AREA_SIZE - 1; _super->cluster_id += 2)
	{
		if (super_cluster_flush(_super->cluster_id) == CLUSTER_NONE)
		{
			ret = 0;
			break;
		}
	}
	return ret;
}

static uint32 super_cluster_load2(uint32 cluster)
{
	uint32 ret = 1;
	if (is_little_endian())
	{
		ret = cluster_read(cluster, (uint8 *)_super);
		if (ret != CLUSTER_NONE)
		{
			ret = cluster_read(cluster + 1, (uint8 *)_super);
		}
	}
	else
	{
		super_cluster *temp = (super_cluster *)malloc(FS_PAGE_SIZE);
		ret = cluster_read(cluster, (uint8 *)temp);
		if (ret != CLUSTER_NONE)
		{
			ret = cluster_read(cluster + 1, (uint8 *)temp);
		}
		if (CLUSTER_NONE == ret)
		{
			os_mem_cpy(_super, temp, FS_PAGE_SIZE);
			_super->flag = convert_endian(temp->flag);
			_super->cluster_id = convert_endian(temp->cluster_id);
			_super->bitmap_id = convert_endian(temp->bitmap_id);
			_super->root_id = convert_endian(temp->root_id);
			_super->backup_id = convert_endian(temp->backup_id);
		}
		free(temp);
	}
	return ret;
}

static uint32 super_cluster_load(uint32 cluster)
{
	uint32 ret = 1;
	if (is_little_endian())
	{
		if (cluster_read(cluster, (uint8 *)_super) == CLUSTER_NONE)
		{
			if (cluster == _super->cluster_id)
			{
				if (cluster_read(cluster + 1, (uint8 *)_super) == CLUSTER_NONE)
				{
					if (cluster == _super->cluster_id)
					{
						ret = 0;
					}
				}
			}
		}
	}
	else
	{
		if (cluster_read(cluster, (uint8 *)_super) == CLUSTER_NONE)
		{
			_super->flag = convert_endian(_super->flag);
			_super->cluster_id = convert_endian(_super->cluster_id);
			_super->bitmap_id = convert_endian(_super->bitmap_id);
			_super->root_id = convert_endian(_super->root_id);
			_super->backup_id = convert_endian(_super->backup_id);
			if (cluster == _super->cluster_id)
			{
				if (cluster_read(cluster + 1, (uint8 *)_super) == CLUSTER_NONE)
				{
					_super->flag = convert_endian(_super->flag);
					_super->cluster_id = convert_endian(_super->cluster_id);
					_super->bitmap_id = convert_endian(_super->bitmap_id);
					_super->root_id = convert_endian(_super->root_id);
					_super->backup_id = convert_endian(_super->backup_id);
					if (cluster == _super->cluster_id)
					{
						ret = 0;
					}
				}
			}
		}
	}
	return ret;
}

static uint32 super_cluster_flush2(uint32 cluster)
{
	uint32 ret = super_cluster_flush(cluster);
	if (ret != CLUSTER_NONE)
	{
		ret = handle_error();
	}
	return ret;
}

static void on_cluster_manager_change(uint32 cluster_id)
{
	_super->bitmap_id = cluster_id;
	super_cluster_flush2(_super->cluster_id);
}

uint32 fs_formatting()
{
	uint32 ret = 1;
	_super = (super_cluster *)malloc(FS_PAGE_SIZE);
	cluster_controler_init();
	register_manager_callback(on_cluster_manager_change);
	super_cluster_init(_super);
	_super->bitmap_id = cluster_manager_init();
	if (_super->bitmap_id > 0)
	{
		for (_super->cluster_id = 1; _super->cluster_id < RETAIN_AREA_SIZE - 1; _super->cluster_id += 2)
		{
			if (super_cluster_flush(_super->cluster_id) == CLUSTER_NONE)
			{
				break;
			}
		}
		if (_super->cluster_id < RETAIN_AREA_SIZE - 1)
		{
			ret = 0;
		}
	}
	return ret;
}

uint32 fs_loading()
{
	uint32 ret = 1;
	uint32 i;
	_super = (super_cluster *)malloc(FS_PAGE_SIZE);
	cluster_controler_init();
	register_manager_callback(on_cluster_manager_change);
	for (i = 1; i < RETAIN_AREA_SIZE - 1; i += 2)
	{
		if (super_cluster_load(i) == CLUSTER_NONE && _super->flag == 0xaa55a55a)
		{
			ret = 0;
			break;
		}
	}
	if (ret != 0)
	{
		for (i = 1; i < RETAIN_AREA_SIZE - 1; i += 2)
		{
			if (super_cluster_load2(i) == CLUSTER_NONE && _super->flag == 0xaa55a55a)
			{
				ret = 0;
				break;
			}
		}
	}
	if (0 == ret)
	{
		ret = cluster_manager_load(_super->bitmap_id);
	}
	if (ret != 0)
	{
		free(_super);
	}
	return ret;
}

void fs_unloading()
{
	uninit();
	if (_super != NULL)
	{
		free(_super);
		_super = NULL;
	}
	if (_root != NULL)
	{
		free(_root);
		_root = NULL;
	}
	if (_backup != NULL)
	{
		free(_backup);
		_backup = NULL;
	}
}

uint32 create_dir()
{
	return 0;
}

uint32 create_file()
{
	return 0;
}

uint32 delete_dir()
{
	return 0;
}

uint32 delete_file()
{
	return 0;
}

uint32 open_file()
{
	return 0;
}

uint32 close_file()
{
	return 0;
}

uint32 read_file()
{
	return 0;
}

uint32 write_file()
{
	return 0;
}

uint32 read_dir()
{
	return 0;
}

uint32 write_dir()
{
	return 0;
}

uint32 find_file()
{
	return 0;
}

void test()
{
	//fs_formatting();
	fs_loading();
	fs_unloading();
}



