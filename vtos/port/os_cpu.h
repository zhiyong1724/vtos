#ifndef __OS_CPU_H__
#define __OS_CPU_H__
#include "os_cfg.h"
#include "os_cpu_def.h"

/*栈增长方向*/
#define  OS_STK_GROWTH        1       //1.从高地址往低地址生长，0.从低地址往高地址生长

/*调度算法相关的定义*/
#define TICK_TIME 10000                  //系统每次滴答的时间间隔，单位为us

/*内存配置项*/
#define PAGE_SIZE 4096                     //内存一页的大小
extern void *_start_addr;                  //交给系统管理内存的起始地址
extern void *_end_addr;                    //交给系统管理内存的结束地址
void os_get_start_addr(void);

/*要用汇编实现的函数*/
os_cpu_sr os_cpu_sr_off(void);               //保存状态寄存器函数,关中断
os_cpu_sr os_cpu_sr_on(void);                //保存状态寄存器函数,开中断
void os_cpu_sr_restore(os_cpu_sr cpu_sr);     //回复状态寄存器函数
void os_ctx_sw(void);                         //任务切换函数
void os_int_ctx_sw(void);                     //中断级任务切换函数
void os_task_start(void);                     //任务开始前准备

/*这部分函数在os_cpu_c.c文件实现*/
void os_mem_init_begin_hook(void);
void os_mem_init_end_hook(os_size_t status);
void os_scheduler_init_begin_hook(void);
void os_scheduler_init_end_hook(os_size_t status);
void os_task_create_hook(void);
void os_task_del_hook(void);
void os_task_idle_hook(void);
void os_task_return_hook(void);
void os_error_msg(char *msg, os_size_t arg);
os_stk *os_task_stk_init(void (*task)(void *p_arg), void *p_arg, os_stk *ptos);
#if (OS_TASK_SW_HOOK_EN > 0)
void os_task_sw_hook(void);
#endif

#if (OS_TIME_TICK_HOOK_EN > 0)
void os_time_tick_hook (void);
#endif
#endif
