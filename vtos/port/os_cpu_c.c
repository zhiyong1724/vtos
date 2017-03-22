#include "os_cpu.h"
#include "sched/os_sched.h"
#ifdef __WINDOWS__
#include <stdlib.h>
#else
extern uint32 _heap_start;
#endif

void os_get_start_addr(void)
{
#ifdef __WINDOWS__
	_start_addr = malloc(0x200000);
	_end_addr = (uint8 *)_start_addr + 0x200000;
#else
	_start_addr = (void *)_heap_start;
	_end_addr = (void *)0x33fffc00;
#endif
}

//内存初始化开始
void os_mem_init_begin_hook(void)
{
}

//内存初始化结束
void os_mem_init_end_hook(os_size_t status)
{
}

//调度器初始化开始
void os_scheduler_init_begin_hook(void)
{
}

//调度器初始化结束
void os_scheduler_init_end_hook(os_size_t status)
{
}

//任务创建Hook
void os_task_create_hook(void)
{
}

//任务删除Hook
void os_task_del_hook(void)
{
}

//空闲任务Hook
void os_task_idle_hook(void)
{
}

//当一个任务返回时调用
void os_task_return_hook(void)
{
}

os_stk *os_task_stk_init(void (*task)(void *p_arg), void *p_arg, os_stk *ptos)
{
	os_stk *stk;
	stk = ptos;
	stk++;
	*(--stk) = (os_stk) task;             //PC
	*(--stk) = (os_stk) os_task_return;   //任务返回地址
	*(--stk) = (os_stk) 0x00000000;       //r12
	*(--stk) = (os_stk) 0x00000000;       //r11
	*(--stk) = (os_stk) 0x00000000;       //r10
	*(--stk) = (os_stk) 0x00000000;       //r9
	*(--stk) = (os_stk) 0x00000000;       //r8

	*(--stk) = (os_stk) 0x00000000;       //r7
	*(--stk) = (os_stk) 0x00000000;       //r6
	*(--stk) = (os_stk) 0x00000000;       //r5
	*(--stk) = (os_stk) 0x00000000;       //r4
	*(--stk) = (os_stk) 0x00000000;       //r3
	*(--stk) = (os_stk) 0x00000000;       //r2
	*(--stk) = (os_stk) 0x00000000;       //r1
	*(--stk) = (os_stk) p_arg;            //r0
	return stk;
}

#if (OS_TASK_SW_HOOK_EN > 0)
void os_task_sw_hook(void)
{
}
#endif

#if (OS_TIME_TICK_HOOK_EN > 0)
void os_time_tick_hook (void)
{
}
#endif

/*void OS_CPU_SysTickHandler (void)
 {
 }*/

#ifdef __WINDOWS__
os_cpu_sr os_cpu_sr_save(void)               //保存状态寄存器函数
{
	return 0;
}
void os_cpu_sr_restore(os_cpu_sr cpu_sr)     //回复状态寄存器函数
{
	cpu_sr = 0;
}
void os_ctx_sw(void)                         //任务切换函数
{
	_runnin_task = _next_task;
}
void os_int_ctx_sw(void)                     //中断级任务切换函数
{
	_runnin_task = _next_task;
}
void os_task_start(void)                     //任务开始前准备
{
	_runnin_task = _next_task;
}
void os_cpu_pend_sv_handler(void)            //软中断入口
{
}
#endif
