#include "disk_driver.h"
#ifdef __WINDOWS__
#include <stdio.h>
#endif // __WINDOWS__
static os_file_operators _file_operators;
#ifdef __WINDOWS__
#define PAGE_SIZE 4096
#define PAGE_COUNT 1024 * 128
static const char *_disk_name = "disk_0";
#endif // __WINDOWS__
static uint32 get_disk_info(os_disk_info *info)
{
#ifdef __WINDOWS__
	info->first_page_id = 0;
	info->page_size = PAGE_SIZE;
	info->page_count = PAGE_COUNT;
	return 0;
#else
#endif // __WINDOWS__
}

static uint32 disk_read(uint32 page_id, void *data)
{
#ifdef __WINDOWS__
	FILE *file;
	if (0 == fopen_s(&file, _disk_name, "rb"))
	{
		fseek(file, PAGE_SIZE * page_id, SEEK_SET);
		fread(data, PAGE_SIZE, 1, file);
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
	FILE *file;
	if (0 == fopen_s(&file, _disk_name, "r+b"))
	{
		fseek(file, PAGE_SIZE * page_id, SEEK_SET);
		fwrite(data, PAGE_SIZE, 1, file);
		fclose(file);
	}
	return 0;
#else
	return 0;
#endif // __WINDOWS__
}

static void module_init(os_file_info *mount_file)
{
#ifdef __WINDOWS__
	FILE *file;
	if (0 == fopen_s(&file, _disk_name, "rb"))
	{
		fclose(file);
	}
	else
	{
		fopen_s(&file, _disk_name, "wb");
		char *buff = (char *)malloc(PAGE_SIZE);
		for (int i = 0; i < PAGE_COUNT; i++)
		{
			fwrite(buff, PAGE_SIZE, 1, file);
		}
		free(buff);
		fclose(file);
	}
#else
#endif // __WINDOWS__
}

static void module_uninit(os_file_info *mount_file)
{

}

static void *ioctl(os_file_info *mount_file, uint32 command, void *arg)
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

static uint32 read_file(os_file_info *mount_file, OS_FILE file, void *data, uint32 len)
{
	return disk_read(len, data);
}

static uint32 write_file(os_file_info *mount_file, OS_FILE file, void *data, uint32 len)
{
	return disk_write(len, data);
}

static OS_FILE open_file(os_file_info *mount_file, const char *path, uint32 flags)
{
	return &_file_operators;
}

static void close_file(os_file_info *mount_file, OS_FILE file)
{
}

uint32 register_disk_dev()
{
	_file_operators.module_init = module_init;
	_file_operators.module_uninit = module_uninit;
	_file_operators.ioctl = ioctl;
	_file_operators.open_file = open_file;
	_file_operators.close_file = close_file;
	_file_operators.read_file = read_file;
	_file_operators.write_file = write_file;
	_file_operators.seek_file = NULL;
	_file_operators.tell_file = NULL;
	return os_register_dev("disk_0", &_file_operators);
}

uint32 unregister_disk_dev()
{
	return os_unregister_dev("disk_0");
}
