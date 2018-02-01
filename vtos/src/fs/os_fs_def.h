#ifndef __OS_FS_DEF_H__
#define __OS_FS_DEF_H__
#include "os_cpu_def.h"
#define FS_PAGE_SIZE 4096
#define FS_MAX_KEY_NUM 16
#define FS_FILE_INFO_SIZE 248
#define FS_MAX_NAME_SIZE 64

typedef struct super_cluster
{
	uint32 flag;
	uint32 cluster_id;
	uint8 name[16];
	uint32 bitmap_id;
	uint32 root_id;
	uint32 backup_id;
} super_cluster;

typedef struct file_info
{
	uint32 cluster_id;
	uint64 size;
	uint32 cluster_count;
	uint64 create_time;
	uint64 modif_time;
	uint32 creator;
	uint32 modifier;
	uint32 limits;
	uint32 backup_id;
	uint32 file_count;
	char name[FS_MAX_NAME_SIZE];
	uint32 non[33];
} file_info;

typedef struct disk_info
{
	uint32 first_page_id;
	uint32 page_size;
	uint32 page_count;
} disk_info;

//下面的函数需要在os_fs_port.c中实现
disk_info os_get_disk_info();
uint32 os_disk_read(uint32 page_id, void *data);
uint32 os_disk_write(uint32 page_id, void *data);
uint64 os_get_time();
#endif