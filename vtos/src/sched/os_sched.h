#ifndef __OS_SCHED_H__
#define __OS_SCHED_H__
#include "base/os_tree.h"
#include "base/os_list.h"
#include "os_timer.h"
#include "base/os_vector.h"
#ifdef __WINDOWS__
#include <windows.h>
#endif
enum task_status_def
{
	TASK_RUNNING,
	TASK_SLEEP,
};

enum event_status_def
{
	EVENT_NONE,
	EVENT_GET_SIGNAL,
	EVENT_WAIT_TIMEOUT
};
extern const os_size_t TREE_NODE_ADDR_OFFSET;     //task_info_t对象中tree_node_structrue地址减去该对象地址
extern const os_size_t LIST_NODE_ADDR_OFFSET;     //task_info_t对象中list_node_structrue地址减去该对象地址
#pragma pack()
typedef struct task_info_t
{
	os_stk *stack;
	tree_node_type_def tree_node_structrue;
	list_node_type_def list_node_structrue;
	os_size_t prio;
	os_size_t unit_vruntime;
	os_size_t vruntime;
	os_size_t task_status;
	os_stk *stack_head;
	os_stk *stack_end;
	os_size_t pid;
	char name[TASK_NAME_SIZE];
	os_size_t event_status;
	timer_info_t timer;
#ifdef __WINDOWS__
	HANDLE handle;
#endif
} task_info_t;
#pragma pack()

struct os_sched
{
	tree_node_type_def *run_task_tree;
	list_node_type_def *end_task_list;
	os_size_t total_task_count;
	os_size_t activity_task_count;
	os_size_t min_vruntime;
	os_vector task_info_index;
	os_size_t running;
	os_size_t cpu_percent;
};

extern task_info_t *_running_task;
#ifdef __WINDOWS__
extern task_info_t *_next_task;
extern struct os_sched _scheduler;
#endif

/*********************************************************************************************************************
* 创建一个新的线程
* task：指向线程函数
* p_arg：传递给线程的参数
* name：线程名称
* return: pid
*********************************************************************************************************************/
os_size_t os_kthread_create(void(*task)(void *p_arg), void *p_arg, const char *name);
/*********************************************************************************************************************
* 创建一个新的线程
* task：指向线程函数
* p_arg：传递给线程的参数
* name：线程名称
* stack_size：指定线程栈的大小
* return: pid
*********************************************************************************************************************/
os_size_t os_kthread_createEX(void(*task)(void *p_arg), void *p_arg, const char *name, os_size_t stack_size);
/*********************************************************************************************************************
* 改变自身的优先级，优先级越高，获得的运行时间越多，默认的优先级为0
* prio：线程的优先级，范围为-20到19
*********************************************************************************************************************/
void os_set_prio(int32 prio);
/*********************************************************************************************************************
* return：返回系统的所有线程数
*********************************************************************************************************************/
os_size_t os_total_thread_count(void);
/*********************************************************************************************************************
* return：返回系统的活动线程数
*********************************************************************************************************************/
os_size_t os_activity_thread_count(void);
/*********************************************************************************************************************
* 该函数是一个系统时钟函数，用来给系统提供时钟节拍，需要定时被调用，或者被映射为中断函数
*********************************************************************************************************************/
void os_sys_tick(void);
/*********************************************************************************************************************
* 返回CPU利用率
*********************************************************************************************************************/
os_size_t os_cpu_percent(void);
/*********************************************************************************************************************
* 开启调度器
*********************************************************************************************************************/
void os_open_scheduler();
/*********************************************************************************************************************
* 关闭调度器，关闭后不能调用os_sleep函数，否则会一直醒不来
*********************************************************************************************************************/
void os_close_scheduler();

void os_init_scheduler(void);
void os_task_return(void);
void os_task_sleep();
void os_task_activity(os_size_t pid);
#endif
