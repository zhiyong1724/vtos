#ifndef __OS_FS_DEF_H__
#define __OS_FS_DEF_H__
#include "os_cpu_def.h"
#endif
#define FS_PAGE_SIZE 4096
#define FS_TNODE_SIZE 128
#define FS_MAX_KEY_NUM 16
#define FS_FILE_INFO_SIZE 248
#define FS_MAX_NAME_SIZE 64

typedef struct tnode
{
	uint32 node_id;
	uint32 pointers[FS_MAX_KEY_NUM + 1];
	uint32 num;     
	uint32 leaf;
	uint32 non[12];
} tnode;

typedef struct file_info
{
	char name[FS_MAX_NAME_SIZE];
	uint8 non[184];
} file_info;

typedef struct fnode
{
	tnode head;
	file_info finfo[FS_MAX_KEY_NUM];

} fnode;

//下面的函数需要在os_fs_port.c中实现
void os_disk_status();
uint32 os_disk_read(uint32 sector_id, void *data);
uint32 os_disk_write(uint32 sector_id, void *data);
void os_get_fattime();