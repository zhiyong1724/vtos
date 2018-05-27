#include "fs/os_fs_def.h"
#include "base/os_string.h"
#include "vtos.h"
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
	//const char *path = get_dev_path(dev_id);
	return 0;
}

uint32 os_disk_read(uint32 page_id, void *data, uint32 dev_id)
{
	//const char *path = get_dev_path(dev_id);
	return 0;
}
uint32 os_disk_write(uint32 page_id, void *data, uint32 dev_id)
{
	//const char *path = get_dev_path(dev_id);
	return 0;
}

uint64 os_get_time()
{
	return os_sys_time();
}
