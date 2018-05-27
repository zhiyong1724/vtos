#include "disk_driver.h"
#ifdef __WINDOWS__
#include <stdio.h>
#endif // __WINDOWS__
static os_file_operators _file_operators;
static uint32 get_disk_info(os_disk_info *info)
{
#ifdef __WINDOWS__
	info->first_page_id = 0;
	info->page_size = 4096;
	info->page_count = 1024 * 32;
	return 0;
#else
#endif // __WINDOWS__
}

static uint32 disk_read(uint32 page_id, void *data)
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
static uint32 disk_write(uint32 page_id, void *data)
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

static void module_init()
{

}

static void module_uninit()
{

}

static void *os_ioctl(uint32 command, void *arg)
{
	switch(command)
	{
	case DISK_INFO:
	{
		static os_disk_info s_info;
		get_disk_info(&s_info);
		return &s_info;
		break;
	}
	default:
	{
		return NULL;
	}
	}
}

static uint32 read_file(OS_FILE file, void *data, uint32 len)
{
	uint32 *page_id = (uint32 *)file;
	return disk_read(*page_id, data);
}

static uint32 write_file(OS_FILE file, void *data, uint32 len)
{
	uint32 *page_id = (uint32 *)file;
	return disk_write(*page_id, data);
}

uint32 register_disk_dev()
{
	_file_operators.module_init = module_init;
	_file_operators.module_uninit = module_uninit;
	_file_operators.os_ioctl = os_ioctl;
	_file_operators.open_file = NULL;
	_file_operators.close_file = NULL;
	_file_operators.read_file = read_file;
	_file_operators.write_file = write_file;
	_file_operators.seek_file = NULL;
	_file_operators.tell_file = NULL;
	return os_register_dev("disk_1", &_file_operators);
}
