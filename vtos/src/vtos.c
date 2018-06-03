#include "vtos.h"
#include "mem/os_mem.h"
#include "sched/os_sched.h"
#include "sched/os_timer.h"
#include "fs/disk_driver.h"
#ifdef __WINDOWS__
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <time.h>
#endif

#include "base/os_string.h"
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
	return endian;
}

uint64 os_sys_time()
{
#ifdef __WINDOWS__
	time_t t = time(NULL);
	return t;
#else
	return 0;
#endif // __WINDOWS__
}

void os_register_devices()
{
	register_disk_dev();
}

void os_unregister_devices()
{
	unregister_disk_dev();
}

void os_mount_root()
{
	os_mount("", "disk_0", "emfs", 1);
}

void os_unmount_root()
{
	os_unmount("");
}

os_size_t os_sys_init(void)
{
	os_size_t ret = 1;
	os_mem_init_begin_hook();
	ret = os_mem_init();
	os_mem_init_end_hook(ret);

	os_init_timer();
	os_sem_init();
	os_q_init();

	os_scheduler_init_begin_hook();
	os_init_scheduler();
	os_scheduler_init_end_hook(ret);

	os_vfs_init();
	os_register_devices();
	os_mount_root();
	return ret;
}

void os_sys_uninit(void)
{
	os_cpu_sr_off();
	os_q_uninit();
	os_sem_uninit();
	os_uninit_timer();
	uninit_os_sched();
	uninit_pid();
	os_unmount_root();
	os_unregister_devices();
	os_vfs_uninit();
}

void os_sys_start(void)
{
	os_task_start();
#ifdef __WINDOWS__
	_cpu_sr = 1;
	for (;;)
	{
		os_sys_tick();
		Sleep(TICK_TIME / 1000);
		while (!_cpu_sr);
	}
#endif // __WINDOWS__
}

#ifdef __WINDOWS__
static void task_a(void *p_arg)
{
	os_q_t *q = os_q_find("test");
	os_size_t status;
	os_size_t v = 0;
	for (;;)
	{
		os_q_pend(q, 0, &status, &v);
		printf("v = %d\n", v);
	}
}

static void task_b(void *p_arg)
{
	printf("This is task b.\n");
	os_task_return();
}

static void task_c(void *p_arg)
{
	os_q_t *q = os_q_create(sizeof(os_size_t), "test");
	os_size_t pid_a = os_kthread_create(task_a, NULL, "task_a");
	for (;;)
	{
		os_sleep(1000);
		os_size_t v = 512;
		os_q_post(q, &v);
		os_size_t pid_b = os_kthread_create(task_b, NULL, "task_b");
	}
}

static void task_d(void *p_arg)
{
	os_size_t i = 0;
	for (;;)
	{
		i++;
		//printf("This is task d.\n");
	}
}

static void task_e(void *p_arg)
{
	os_q_t *q = os_q_create(sizeof(os_size_t), "test1");
	os_size_t pid_d = os_kthread_create(task_d, NULL, "task_d");
	os_size_t status;
	os_size_t v = 0;
	for (;;)
	{
		os_q_pend(q, 500, &status, &v);
		os_suspend_thread(pid_d);
		os_q_pend(q, 5000, &status, &v);
		os_resume_thread(pid_d);
	}
}

static DWORD task(LPVOID lpThreadParameter)
{
	os_kthread_create(task_c, NULL, "task_c");
	os_kthread_create(task_e, NULL, "task_e");
	os_sys_start();
	return 0;
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
	char buff[256] = "";
	char command[16] = "";
	char arg1[256] = "";
	char arg2[256] = "";
	char ln;
	//HANDLE handle;
	//_CrtSetBreakAlloc(96);
	if (0 == os_sys_init())
	{
		/*handle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)task, NULL, 0, NULL);
		SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);*/
	}
	/*if (fs_loading() != 0)
	{
		printf("磁盘可能没有格式化，是否要进行格式化？y/n\n");
		scanf_s("%[^\n]%c", buff, 256, &ln, 1);
		if (os_str_cmp(buff, "y") == 0)
		{
			fs_formatting();
			printf("ok\n");
		}
		else
		{
			_CrtDumpMemoryLeaks();
			return 0;
		}
	}*/
	while (1)
	{
		scanf_s("%[^\n]%c", buff, 256, &ln, 1);
		get_command(buff, command, arg1, arg2);
		if (os_str_cmp(command, "formatting") == 0)
		{
			os_formatting("");
			printf("ok\n");
		}
		else if (os_str_cmp(command, "df") == 0)
		{
			uint64 all_cluster = os_total_size("");
			uint64 free_cluster = os_free_size("");
			printf("总大小：%lld\n", all_cluster);
			printf("剩余大小：%lld\n", free_cluster);
		}
		else if (os_str_cmp(command, "ls") == 0)
		{
 			os_dir *dir = os_open_dir(arg1);
			if (dir != NULL)
			{
				os_file_info *finfo = (os_file_info *)os_malloc(sizeof(os_file_info));
				printf("名字    大小    创建时间    修改时间    创建者    修改者\n");
				while (os_read_dir(finfo, dir) == 0)
				{
					printf("%s %lld %lld %lld %d %d\n", finfo->name, finfo->size, finfo->create_time, finfo->modif_time, finfo->creator, finfo->modifier);
				}
				os_close_dir(dir);
				os_free(finfo);
			}
			else
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "mkdir") == 0)
		{
			if (os_create_dir(arg1) == 0)
			{
				printf("ok\n");
			}
			else
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "mkvfile") == 0)
		{
			if (os_create_vfile(arg1, NULL, NULL) == 0)
			{
				printf("ok\n");
			}
			else
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "rmvfile") == 0)
		{
			if (os_delete_vfile(arg1) == 0)
			{
				printf("ok\n");
			}
			else
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "touch") == 0)
		{
			if (os_create_file(arg1) == 0)
			{
				printf("ok\n");
			}
			else
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "rmdir") == 0)
		{
			if (os_delete_dir(arg1) == 0)
			{
				printf("ok\n");
			}
			else
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "rm") == 0)
		{
			if (os_delete_file(arg1) == 0)
			{
				printf("ok\n");
			}
			else
			{
				printf("error\n");
			}
		}
		else if (os_str_cmp(command, "mv") == 0)
		{
			/*if (move_file(arg2, arg1) == 0)
			{
				printf("ok\n");
			}
			else
			{
				printf("error\n");
			}*/
		}
		else if (os_str_cmp(command, "push") == 0)
		{
			/*FILE *file1 = NULL;
			if (0 == fopen_s(&file1, arg1, "rb"))
			{
				file_obj *file2 = open_file(arg2, FS_CREATE | FS_WRITE | FS_APPEND);
				if (file2 != NULL)
				{
					char buff[4096];
					int num;
					long long size = 0;
					long long cur = 0;
					fseek(file1, 0, SEEK_END);
					size = ftell(file1);
					fseek(file1, 0, SEEK_SET);
					while ((num = fread_s(buff, 4096, 1, 4096, file1)) > 0)
					{
						write_file(file2, buff, num);
						cur += num;
						printf("%lld%%\r", cur * 100 / size);
					}
					printf("\n%s to %s\n", arg1, arg2);
					close_file(file2);
				}
				fclose(file1);
			}*/

		}
		else if (os_str_cmp(command, "pull") == 0)
		{
			/*FILE *file2 = NULL;
			if (0 == fopen_s(&file2, arg2, "wb"))
			{
				file_obj *file1 = open_file(arg1, FS_READ);
				if (file1 != NULL)
				{
					char buff[4096];
					int num;
					long long size = 0;
					long long cur = 0;
					seek_file(file1, 0, FS_SEEK_END);
					size = tell_file(file1);
					seek_file(file1, 0, FS_SEEK_SET);
					while((num = read_file(file1, buff, 4096)) > 0)
					{
						fwrite(buff, 1, num, file2);
						cur += num;
						printf("%lld%%\r", cur * 100 / size);
					}
					printf("\n%s to %s\n", arg1, arg2);
					close_file(file1);
				}
				fclose(file2);
			}*/

		}
		else if (os_str_cmp(command, "test") == 0)
		{
			int i;
			char n[256];
			for (i = 1; i <= 1000; i++)
			{
				sprintf_s(n, 256, "/a/b/%d", i);
				if (os_create_file(n) != 0)
				{
					i = i;
				}
			}
			for (i = 1000; i > 0; i--)
			{
				sprintf_s(n, 256, "/a/b/%d", i);
				if (os_delete_file(n) != 0)
				{
					i = 1;
				}
			}
			printf("ok\n");
		}
		else if (os_str_cmp(command, "quit") == 0)
		{
			os_sys_uninit();
			/*CloseHandle(handle);
			fs_unloading();*/
			break;
		}
		else
		{
			printf("command not found\n");
		}
	}
	_CrtDumpMemoryLeaks();
	return 0;
}
#endif
