#ifndef _OS_TIMER_H
#define _OS_TIMER_H
#include "os_cpu.h"
#include "base/os_tree.h"
typedef void (*timer_call_back)(void *p_arg);
typedef struct timer_info_t
{
	tree_node_type_def tree_node_structrue;
	uint64 tick;
	timer_call_back call_back_func;
	void *args;
} timer_info_t;

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
