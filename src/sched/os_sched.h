#ifndef __OS_SCHED_H__
#define __OS_SCHED_H__
#include "base/os_tree.h"
#include "base/os_list.h"
#include "base/os_vector.h"
#include "os_pid.h"
#include "os_timer.h"
#ifdef __WINDOWS__
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define TASK_NAME_SIZE        32
	enum task_status_def
	{
		TASK_RUNNING,
		TASK_SUSPEND,
		TASK_WAIT,
		TASK_SLEEP,
		TASK_DELETE,
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
		timer_info_t timer;
		int8 prio;
		os_size_t unit_vruntime;
		os_size_t vruntime;
		os_size_t task_status;
		os_stk *stack_head;
		os_stk *stack_end;
		os_size_t pid;
		char name[TASK_NAME_SIZE];
		os_size_t event_status;
		void *sem;
#ifdef __WINDOWS__
		HANDLE handle;
#endif
	} task_info_t;
#pragma pack()

	typedef struct task_info
	{
		int8 prio;
		os_size_t task_status;
		os_size_t pid;
		char name[TASK_NAME_SIZE];
	} task_info;

	struct os_sched
	{
		tree_node_type_def *run_task_tree;
		list_node_type_def *end_task_list;
		list_node_type_def *suspend_task_list;
		os_size_t total_task_count;
		os_size_t activity_task_count;
		os_size_t min_vruntime;
		os_vector task_info_index;
		os_size_t running;
		os_size_t cpu_percent;
		os_size_t sw_total_count;
		os_size_t sw_idle_count;
		uint64 tick;
	};

	extern task_info_t *_running_task;
	extern struct os_sched _scheduler;
#ifdef __WINDOWS__
	extern task_info_t *_next_task;
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
	* prio：线程的优先级，范围为-20到19,默认优先级为0
	*********************************************************************************************************************/
	void os_set_prio(int8 prio);
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
	/*********************************************************************************************************************
	* 删除线程
	* pid 线程id
	* return：0：删除成功
	*********************************************************************************************************************/
	os_size_t os_delete_thread(os_size_t pid);
	/*********************************************************************************************************************
	* 挂起线程
	* pid 线程id
	* return：0：挂起成功
	*********************************************************************************************************************/
	os_size_t os_suspend_thread(os_size_t pid);
	/*********************************************************************************************************************
	* 恢复线程
	* pid 线程id
	* return：0：恢复成功
	*********************************************************************************************************************/
	os_size_t os_resume_thread(os_size_t pid);
	/*********************************************************************************************************************
	* 获得tick
	* return：tick
	*********************************************************************************************************************/
	uint64 os_get_tick();
	/*********************************************************************************************************************
	* 获得任务信息
	* out：返回任务信息
	*********************************************************************************************************************/
	void os_get_task_info(task_info *out);
	/*********************************************************************************************************************
	* 获得任务信息
	* pid
	* out：返回任务信息
	* return 0：返回成功
	*********************************************************************************************************************/
	os_size_t os_get_task_info_by_pid(task_info *out, os_size_t pid);
	/*********************************************************************************************************************
	* 线程退出之前需要调用，回收资源
	*********************************************************************************************************************/
	void os_task_return(void);

	void os_init_scheduler(void);
	void os_sw_out();
	void os_sw_in(task_info_t *task);
	void os_insert_runtree(task_info_t *task);
	void os_free_task_info(task_info_t *task);
	void uninit_os_sched(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif