#include "fs/os_fs_def.h"
#include "vtos.h"
#ifdef __WINDOWS__
#include <stdio.h>
#include <string.h>
#endif

uint32 os_get_disk_info(disk_info *info, uint32 dev_id)
{
#ifdef __WINDOWS__
	info->first_page_id = 0;
	info->page_size = FS_CLUSTER_SIZE;
	info->page_count = 1024 * 32;
	return 0;
#else
#endif // __WINDOWS__
}

uint32 os_disk_read(uint32 page_id, void *data, uint32 dev_id)
{
#ifdef __WINDOWS__
	char name[32];
	FILE *file;
	sprintf_s(name, 32, "%d", page_id);
	if (0 == fopen_s(&file, name, "rb"))
	{
		fread(data, FS_CLUSTER_SIZE, 1, file);
		fclose(file);
	}
	return 0;
#else
	return 0;
#endif // __WINDOWS__
}
uint32 os_disk_write(uint32 page_id, void *data, uint32 dev_id)
{
#ifdef __WINDOWS__
	char name[32];
	FILE *file;
	sprintf_s(name, 32, "%d", page_id);
	if (0 == fopen_s(&file, name, "wb"))
	{
		fwrite(data, FS_CLUSTER_SIZE, 1, file);
		fclose(file);
	}
	return 0;
#else
	return 0;
#endif // __WINDOWS__
}

uint64 os_get_time()
{
	return os_sys_time();
}
