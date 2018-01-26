#ifndef __OS_TIMER_H__
#define __OS_TIMER_H__
#include "os_cpu.h"
#include "base/os_tree.h"
#include "sched/os_timer_def.h"
struct timer_controler_t
{
	tree_node_type_def *timer_tree;
	uint64 tick;
	timer_info_t *min_timer;
};

timer_info_t *os_set_timer(timer_info_t *timer, uint64 time, timer_call_back call_back_func, void *args);
void os_close_timer(timer_info_t *timer_info);
void os_init_timer(void);
void os_timer_tick(void);
#endif
