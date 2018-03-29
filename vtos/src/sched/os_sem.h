#ifndef __OS_SEM_H__
#define __OS_SEM_H__
#include "sched/os_timer.h"
#include "base/os_list.h"
typedef struct os_sem_t
{
	uint32 sem;
	list_node_type_def *wait_task_list;
	timer_info_t timer;
} os_sem_t;
#endif
