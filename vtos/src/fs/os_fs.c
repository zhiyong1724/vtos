#include "fs/os_fs.h"
#include "lib/os_string.h"
#include "fs/os_dentry.h"
#include <vtos.h>
#include <stdlib.h>
static super_cluster *_super_cluster = NULL;
static fnode *_root = NULL;
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

static uint32 super_cluster_flush(uint32 cluster, super_cluster *super)
{
	uint32 ret = CLUSTER_NONE;
	if (is_little_endian())
	{
		ret = cluster_write(cluster, (uint8 *)super);
	}
	else
	{
		super_cluster *temp = (super_cluster *)malloc(FS_PAGE_SIZE);
		os_mem_cpy(temp, super, FS_PAGE_SIZE);

		temp->flag = convert_endian(super->flag);
		temp->bitmap_id = convert_endian(super->bitmap_id);
		temp->root_id = convert_endian(super->root_id);
		temp->backup_id = convert_endian(super->backup_id);
		ret = cluster_write(cluster, (uint8 *)temp);
		free(temp);
	}
	return ret;
}

uint32 formatting()
{
	uint32 i;
	super_cluster *super = (super_cluster *)malloc(FS_PAGE_SIZE);
	super_cluster_init(super);
	super->bitmap_id = cluster_manager_init();
	if (super->bitmap_id > 0)
	{
		super->root_id = 0;
		super->backup_id = 0;
	}
	for (i = 1; i < RETAIN_AREA_SIZE - 1; i += 2)
	{
		if (super_cluster_flush(i, super) == CLUSTER_NONE && super_cluster_flush(i + 1, super) == CLUSTER_NONE)
		{
			break;
		}
	}
	if (i < RETAIN_AREA_SIZE - 1)
	{
		return 1;
	}
	free(super);
	return 0;
}

void test()
{
	formatting();
}



