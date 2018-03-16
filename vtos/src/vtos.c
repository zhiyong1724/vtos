#include "vtos.h"
#include "mem/os_mem.h"
#include "sched/os_sched.h"
#include "sched/os_timer.h"
#include "lib/os_string.h"
#include "fs/os_fs.h"
#ifdef __WINDOWS__
#define _CRTDBG_MAP_ALLOC
#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>
#endif
const char *VERSION = "0.0.2";

const char *os_version(void)
{
	return VERSION;
}

uint32 is_little_endian()
{
	static uint32 endian = 2;
	if (2 == endian)
	{
		union temp
		{
			uint32 i;
			uint8 c;
		} t;
		t.i = 1;
		if (1 == t.c)
		{
			endian = 1;
		}
		else
		{
			endian = 0;
		}
	}
	return !endian;
}

os_size_t os_sys_init(void)
{
	os_size_t ret = 1;
	os_mem_init_begin_hook();
	ret = os_mem_init();
	os_mem_init_end_hook(ret);

	os_scheduler_init_begin_hook();
	ret = os_init_scheduler();
	os_scheduler_init_end_hook(ret);
	os_init_timer();
	return ret;
}

void os_sys_start(void)
{
	os_task_start();
}

#ifdef __WINDOWS__
static void task(void *p_arg)
{
	for (;;)
	{

	}
}

static void get_command(const char *src, char *command, char *arg1, char *arg2)
{
	for (; *src != '\0' && *src != ' '; src++, command++)
	{
		*command = *src;
	}
	*command = '\0';
	for (; *src != '\0' && *src == ' ';)
	{
		src++;
	}
	for (; *src != '\0' && *src != ' '; src++, arg1++)
	{
		*arg1 = *src;
	}
	*arg1 = '\0';
	for (; *src != '\0' && *src == ' ';)
	{
		src++;
	}
	for (; *src != '\0' && *src != ' '; src++, arg2++)
	{
		*arg2 = *src;
	}
	*arg2 = '\0';
	for (; *src != '\0' && *src == ' ';)
	{
		src++;
	}
}

int main()
{
	//_CrtSetBreakAlloc(86);
	//if (0 == os_sys_init())
	//{
		/*os_kthread_create(task, NULL, "task_a");
		os_sys_start();
		while(1)
		{
			os_sys_tick();
		}*/

	//}
	char buff[256] = "";
	char command[16] = "";
	char arg1[256] = "";
	char arg2[256] = "";
	char ln;
	fs_formatting();
	fs_loading();
	while (os_str_cmp(buff, "quit") != 0)
	{
		scanf_s("%[^\n]%c", buff, 256, &ln, 1);
		get_command(buff, command, arg1, arg2);
		if (os_str_cmp(command, "df") == 0)
		{
			int all_cluster = get_all_cluster_num();
			int free_cluster = get_free_cluster_num();
			printf("总大小：%d\n", all_cluster * FS_PAGE_SIZE);
			printf("剩余大小：%d\n", free_cluster * FS_PAGE_SIZE);
		}
		else if (os_str_cmp(command, "ls") == 0)
		{
			dir_obj *dir = open_dir(arg1);
			if (dir != NULL)
			{
				file_info *finfo = (file_info *)malloc(sizeof(file_info));
				printf("名字    大小    占用簇    文件数    创建时间    修改时间    创建者    修改者\n");
				while (read_dir(finfo, dir) == 0)
				{
					printf("%s %lld %d %d %lld %lld %d %d\n", finfo->name, finfo->size, finfo->cluster_count, finfo->file_count, finfo->create_time, finfo->modif_time, finfo->creator, finfo->modifier);
				}
				close_dir(dir);
				free(finfo);
			}
			else
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "mkdir") == 0)
		{
			if (create_dir(arg1) != 0)
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "touch") == 0)
		{
			if (create_file(arg1) != 0)
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "rmdir") == 0)
		{
			if (delete_dir(arg1) != 0)
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "rmfile") == 0)
		{
			if (delete_file(arg1) != 0)
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "mv") == 0)
		{
			if (move_file(arg2, arg1) != 0)
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "push") == 0)
		{
			FILE *file1 = NULL;
			if (0 == fopen_s(&file1, arg1, "rb"))
			{
				file_obj *file2 = open_file(arg2, FS_CREATE | FS_WRITE | FS_APPEND);
				if (file2 != NULL)
				{
					char buff[1024];
					int num;
					while ((num = fread_s(buff, 1024, 1, 1024, file1)) > 0)
					{
						write_file(file2, buff, num);
					}
					printf("%s to %s", arg1, arg2);
					close_file(file2);
				}
				fclose(file1);
			}

		}
		else if (os_str_cmp(command, "pull") == 0)
		{
			FILE *file2 = NULL;
			if (0 == fopen_s(&file2, arg2, "wb"))
			{
				file_obj *file1 = open_file(arg1, FS_READ);
				if (file1 != NULL)
				{
					char buff[1024];
					int num;
					while((num = read_file(file1, buff, 1024)) > 0)
					{
						fwrite(buff, 1, num, file2);
					}
					printf("%s to %s", arg1, arg2);
					close_file(file1);
				}
				fclose(file2);
			}

		}
		else if (os_str_cmp(command, "test") == 0)
		{
			int i;
			char n[16];
			for (i = 0; i < 512; i++)
			{
				sprintf_s(n, 16, "/a/%d", i);
				create_dir(n);
			}
			
		}
		else
		{
			printf("command not found\n");
		}
	}
	fs_unloading();
	_CrtDumpMemoryLeaks();
	return 0;
}
#endif
