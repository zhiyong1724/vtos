#ifndef _OS_CPU_H
#define _OS_CPU_H
#include "os_cfg.h"
#ifndef NULL
#define NULL 0
#endif
/*与编译器相关的类型定义*/
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long uint64;
typedef signed long int64;
typedef unsigned int os_stk; //堆栈类型，堆栈存取宽度
typedef unsigned int os_cpu_sr; //用于保存CPU状态寄存器类型
typedef unsigned int os_size_t; //这个类型要保证能放下一个地址

/*栈增长方向*/
#define  OS_STK_GROWTH        1       //1.从高地址往低地址生长，0.从低地址往高地址生长

/*调度算法相关的定义*/
#define TICK_TIME 10000                  //系统每次滴答的时间间隔，单位为us

/*内存配置项*/
#define PAGE_SIZE 512                      //内存管理的最小单位，按页管理
extern void *_start_addr;                  //交给系统管理内存的起始地址
extern void *_end_addr;                    //交给系统管理内存的结束地址
void os_get_start_addr(void);

/*要用汇编实现的函数*/
os_cpu_sr os_cpu_sr_save(void);               //保存状态寄存器函数,关中断
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
os_stk *os_task_stk_init(void (*task)(void *p_arg), void *p_arg, os_stk *ptos);
#if (OS_TASK_SW_HOOK_EN > 0)
void os_task_sw_hook(void);
#endif

#if (OS_TIME_TICK_HOOK_EN > 0)
void os_time_tick_hook (void);
#endif
#endif
