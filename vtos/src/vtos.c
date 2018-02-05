#include "vtos.h"
#include "mem/os_mem.h"
#include "sched/os_sched.h"
#include "sched/os_timer.h"
#include "lib/os_string.h"
#include "fs/os_fs.h"
#ifdef __WINDOWS__
#define _CRTDBG_MAP_ALLOC
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
void task(void *p_arg)
{
	for (;;)
	{

	}
}

int main()
{
	//_CrtSetBreakAlloc(135);
	//if (0 == os_sys_init())
	//{
		test();
		/*os_kthread_create(task, NULL, "task_a");
		os_sys_start();
		while(1)
		{
			os_sys_tick();
		}*/

	//}
	_CrtDumpMemoryLeaks();
	return 0;
}
#endif
