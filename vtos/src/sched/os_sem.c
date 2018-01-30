#include "sched/os_sem.h"
#include "vtos.h"
#include "sched/os_sched.h"
os_sem_t *os_sem_create(os_sem_t *p_sem, uint32 cnt)
{
	os_cpu_sr cpu_sr = os_cpu_sr_off();
	p_sem->sem = cnt;
	p_sem->wait_task_list = NULL;
	os_cpu_sr_restore(cpu_sr);
	return p_sem;
}

static void task_wakeup(void *args)
{
	os_sem_t *p_sem = (os_sem_t *)args;
	task_info_t *task_info = (task_info_t *)((uint8 *)os_get_back_from_list(&(p_sem->wait_task_list)) - LIST_NODE_ADDR_OFFSET);
	os_remove_from_list(&(p_sem->wait_task_list), &task_info->list_node_structrue);
	task_info->event_status = EVENT_WAIT_TIMEOUT;
	os_task_activity(task_info->pid);
}

void os_sem_pend(os_sem_t *p_sem, uint64 timeout, uint32 *p_status)
{
	os_cpu_sr cpu_sr = os_cpu_sr_off();
	if (p_sem->sem > 0)
	{
		p_sem->sem--;
		*p_status = EVENT_GET_SIGNAL;
		os_cpu_sr_restore(cpu_sr);
	}
	else
	{
		os_set_timer(&p_sem->timer, timeout, task_wakeup, p_sem);
		os_insert_to_front(&(p_sem->wait_task_list), &_running_task->list_node_structrue);
		os_cpu_sr_restore(cpu_sr);
		os_task_sleep();
		cpu_sr = os_cpu_sr_off();
		os_close_timer(&p_sem->timer);
		*p_status = _running_task->event_status;
		_running_task->event_status = EVENT_NONE;
		os_cpu_sr_restore(cpu_sr);
	}
}

uint32 os_sem_post(os_sem_t *p_sem)
{
	os_cpu_sr cpu_sr = os_cpu_sr_off();
	if (p_sem != NULL)
	{
		if (p_sem->wait_task_list != NULL)
		{
			task_info_t *task_info = (task_info_t *)((uint8 *)os_get_back_from_list(&(p_sem->wait_task_list)) - LIST_NODE_ADDR_OFFSET);
			os_remove_from_list(&(p_sem->wait_task_list), &task_info->list_node_structrue);
			task_info->event_status = EVENT_GET_SIGNAL;
			os_task_activity(task_info->pid);
		}
		else
		{
			p_sem->sem++;
		}
	}
	os_cpu_sr_restore(cpu_sr);
	return p_sem->sem;
}
