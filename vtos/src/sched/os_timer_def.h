#ifndef __OS_TIMER_DEF_H__
#define __OS_TIMER_DEF_H__
#include "base/os_tree_def.h"
typedef void(*timer_call_back)(void *p_arg);
typedef struct timer_info_t
{
	tree_node_type_def tree_node_structrue;
	uint64 tick;
	timer_call_back call_back_func;
	void *args;
} timer_info_t;

#endif