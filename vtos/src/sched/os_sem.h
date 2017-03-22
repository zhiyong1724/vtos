#ifndef _OS_SEM_H
#define _OS_SEM_H
#include "sched/os_sched.h"
typedef struct os_sem_t
{
	uint32 sem;
	list_node_type_def *wait_task_list;
	timer_info_t timer;
} os_sem_t;
#endif
