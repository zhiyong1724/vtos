#include "fs/os_fs_def.h"
#include "base/os_string.h"
#include "vtos.h"
#ifdef __WINDOWS__
#include <stdio.h>
#endif // __WINDOWS__
static const char *_dev_path = "/dev/disk_";

static const char *get_dev_path(uint32 dev_id)
{
	static char path[16];
	char num[8];
	os_utoa(num, (os_size_t)dev_id);
	os_str_cpy(path, _dev_path, 16);
	os_str_cat(path, num);
	return path;
}

uint32 os_get_disk_info(disk_info *info, uint32 dev_id)
{
#ifdef __WINDOWS__
	info->first_page_id = 0;
	info->page_size = 4096;
	info->page_count = 1024 * 32;
	return 0;
#else
	return 0;
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
		fread(data, 4096, 1, file);
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
		fwrite(data, 4096, 1, file);
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
