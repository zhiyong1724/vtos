#ifndef _OS_SEM_DEF_H
#define _OS_SEM_DEF_H
#include "sched/os_timer_def.h"
#include "base/os_list_def.h"
typedef struct os_sem_t
{
	uint32 sem;
	list_node_type_def *wait_task_list;
	timer_info_t timer;
} os_sem_t;
#endif