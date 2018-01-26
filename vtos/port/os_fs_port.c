#include "fs/os_fs_def.h"
#ifdef __WINDOWS__
#include <stdio.h>
#include <string.h>
#endif
void os_disk_status()
{

}
uint32 os_disk_read(uint32 sector_id, void *data)
{
#ifdef __WINDOWS__
	char name[32];
	FILE *file;
	sprintf_s(name, 32, "%d", sector_id);
	if (0 == fopen_s(&file, name, "rb"))
	{
		fread(data, FS_PAGE_SIZE, 1, file);
		fclose(file);
	}
	return 0;
#else
	return 0;
#endif // __WINDOWS__
}
uint32 os_disk_write(uint32 sector_id, void *data)
{
#ifdef __WINDOWS__
	char name[32];
	FILE *file;
	sprintf_s(name, 32, "%d", sector_id);
	if (0 == fopen_s(&file, name, "wb"))
	{
		fwrite(data, FS_PAGE_SIZE, 1, file);
		fclose(file);
	}
	return 0;
#else
	return 0;
#endif // __WINDOWS__
}
void os_get_fattime()
{

}