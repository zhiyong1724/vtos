#ifndef __OS_TIMER_H__
#define __OS_TIMER_H__
#include "os_cpu.h"
#include "base/os_tree.h"

typedef void(*timer_call_back)(void *p_arg);
#pragma pack()
typedef struct timer_info_t
{
	tree_node_type_def tree_node_structrue;
	os_size_t tick;
	timer_call_back call_back_func;
	void *args;
} timer_info_t;
#pragma pack()

struct os_timer
{
	tree_node_type_def *timer_tree;
	os_size_t tick;
	timer_info_t *min_timer;
};

/*********************************************************************************************************************
* 线程睡眠函数
* 睡眠的时间，单位为ms
*********************************************************************************************************************/
VTOS_API void os_sleep(os_size_t t);

void os_remove_task_from_timer(tree_node_type_def *node);
timer_info_t *os_set_timer(timer_info_t *timer, os_size_t time, timer_call_back call_back_func, void *args);
void os_close_timer(timer_info_t *timer_info);
void os_init_timer(void);
void os_uninit_timer(void);
void os_timer_tick(void);
#endif
