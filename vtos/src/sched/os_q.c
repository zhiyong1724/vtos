#include "vtos.h"
#include "sched/os_q.h"
os_q_t *os_q_create(os_q_t *p_q, void **pp_start, uint32 ms_count)
{
	os_sem_create(&p_q->sem, 0, "");
	p_q->total_ms_count = ms_count;
	p_q->cur_ms_count = 0;
	p_q->pp_start_index = pp_start;
	p_q->pp_push_index = pp_start;
	p_q->pp_pop_index = pp_start;
	return p_q;
}

void os_q_reset(os_q_t *p_queue)
{
	p_queue->pp_push_index = p_queue->pp_start_index;
	p_queue->pp_pop_index = p_queue->pp_start_index;
	p_queue->cur_ms_count = 0;
}

void *os_q_pend(os_q_t *p_queue, uint32 timeout, uint32 *p_status)
{
	void *ret = NULL;
	os_sem_pend(&p_queue->sem, timeout, p_status);
	if (EVENT_GET_SIGNAL == *p_status)
	{
		ret = *(p_queue->pp_pop_index);
		p_queue->pp_pop_index++;
		p_queue->cur_ms_count--;
		if (p_queue->pp_pop_index > p_queue->pp_start_index + p_queue->total_ms_count - 1)
		{
			p_queue->pp_pop_index = p_queue->pp_start_index;
		}
	}
	return ret;
}

uint32 os_q_post(os_q_t *p_queue, void *p_msg)
{
	if (p_queue->cur_ms_count < p_queue->total_ms_count)
	{
		if (p_queue->pp_push_index > p_queue->pp_start_index + p_queue->total_ms_count - 1)
		{
			p_queue->pp_push_index = p_queue->pp_start_index;
		}
		*p_queue->pp_push_index = p_msg;
		p_queue->pp_push_index++;
		p_queue->cur_ms_count++;
		os_sem_post(&p_queue->sem);
	}
	return (p_queue->sem.sem);
}
