#include "vtos.h"
#include "mem/os_mem.h"
#include "sched/os_sched.h"
#include "sched/os_timer.h"
#include "lib/os_string.h"
const char *VERSION = "0.0.1";

const char *os_version(void)
{
	return VERSION;
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
	if (0 == os_sys_init())
	{
		os_kthread_create(task, NULL, "task_a");
		os_sys_start();
		while(1)
		{
			os_sys_tick();
		}
		
	}
	return 0;
}
#endif
