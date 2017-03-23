#ifndef _OS_Q_DEF_H
#define  _OS_Q_DEF_H
#include "sched/os_sem_def.h"
typedef struct os_q_t
{
	void **pp_start_index;
	void **pp_push_index;
	void **pp_pop_index;
	uint32 total_ms_count;
	uint32 cur_ms_count;
	os_sem_t sem;
} os_q_t;
#endif