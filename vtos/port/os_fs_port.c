#include "fs/os_fs_def.h"
#ifdef __WINDOWS__
#include <stdio.h>
#include <string.h>
#include <time.h>
#endif

disk_info os_get_disk_info()
{
#ifdef __WINDOWS__
	disk_info info;
	info.first_page_id = 0;
	info.page_size = FS_PAGE_SIZE;
	info.page_count = 1024 * 64;
	return info;
#else
#endif // __WINDOWS__
}

uint32 os_disk_read(uint32 page_id, void *data)
{
#ifdef __WINDOWS__
	char name[32];
	FILE *file;
	sprintf_s(name, 32, "%d", page_id);
	if (0 == fopen_s(&file, name, "rb"))
	{
		fread(data, FS_PAGE_SIZE, 1, file);
		fclose(file);
		return 0;
	}
	return 1;
#else
	return 0;
#endif // __WINDOWS__
}
uint32 os_disk_write(uint32 page_id, void *data)
{
#ifdef __WINDOWS__
	char name[32];
	FILE *file;
	sprintf_s(name, 32, "%d", page_id);
	if (0 == fopen_s(&file, name, "wb"))
	{
		fwrite(data, FS_PAGE_SIZE, 1, file);
		fclose(file);
		return 0;
	}
	return 1;
#else
	return 0;
#endif // __WINDOWS__
}

uint64 os_get_time()
{

#ifdef __WINDOWS__
	time_t t = time(NULL);
	return t;
#else
	return 0;
#endif // __WINDOWS__
}
