#ifndef __OS_SCHED_H__
#define __OS_SCHED_H__
#include "base/os_tree.h"
#include "base/os_list.h"
#include "os_timer.h"
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
typedef struct task_info_t
{
	os_stk *stack;
	tree_node_type_def tree_node_structrue;
	list_node_type_def list_node_structrue;
	int32 prio;
	uint64 unit_vruntime;
	uint64 vruntime;
	uint32 task_status;
	os_stk *stack_head;
	os_stk *stack_end;
	uint32 pid;
	char name[TASK_NAME_SIZE];
	int32 event_status;
	timer_info_t timer;
} task_info_t;

struct scheduler_info
{
	tree_node_type_def *run_task_tree;
	list_node_type_def *end_task_list;
	uint32 total_task_count;
	uint32 activity_task_count;
	uint64 min_vruntime;
	task_info_t *task_info_index[MAX_PID_COUNT];
	uint32 running;
};

extern task_info_t *_running_task;
#ifdef __WINDOWS__
extern task_info_t *_next_task;
extern struct scheduler_info _scheduler;
#endif

os_size_t os_init_scheduler(void);
void os_task_return(void);
void os_task_sleep();
void os_task_activity(uint32 pid);
#endif
