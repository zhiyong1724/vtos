#include "sched/os_sem.h"
#include "vtos.h"
#include "sched/os_sched.h"
#include "base/os_string.h"
static struct os_sem _os_sem;
static int8 insert_compare(void *key1, void *key2, void *arg)
{
	os_sem_t *info1 = (os_sem_t *)key1;
	os_sem_t *info2 = (os_sem_t *)key2;
	int8 results = os_str_cmp(info1->name, info2->name);
	return results;
}

static int8 find_compare(void *key1, void *key2, void *arg)
{
	char *name = (char *)key1;
	os_sem_t *info = (os_sem_t *)key2;
	int8 results = os_str_cmp(name, info->name);
	return results;
}

void os_sem_init()
{
	_os_sem.tree = NULL;
}

void os_sem_uninit()
{
	while (_os_sem.tree != NULL)
	{
		os_sem_t *sem = (os_sem_t *)_os_sem.tree;
		while (sem->wait_task_list != NULL)
		{
			task_info_t *task = (task_info_t *)((uint8 *)sem->wait_task_list - LIST_NODE_ADDR_OFFSET);
			os_remove_from_list(&sem->wait_task_list, sem->wait_task_list);
			os_free_task_info(task);
		}
		os_delete_node(&_os_sem.tree, _os_sem.tree);
		os_kfree(sem);
	}
}

os_sem_t *os_sem_create(os_size_t cnt, const char *name)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	os_sem_t *p_sem = (os_sem_t *)os_kmalloc(sizeof(os_sem_t));
	p_sem->sem = cnt;
	p_sem->wait_task_list = NULL;
	if (name != NULL && name[0] != '\0')
	{
		os_str_cpy(p_sem->name, name, SEM_NAME_SIZE);
		if (os_insert_node(&_os_sem.tree, &p_sem->tree, insert_compare, NULL) != 0)
		{
			os_kfree(p_sem);
			p_sem = NULL;
		}
	}
	else
	{
		p_sem->name[0] = '\0';
	}
	os_cpu_sr_restore(cpu_sr);
	return p_sem;
}

static void task_wakeup(void *args)
{
	os_sem_t *p_sem = (os_sem_t *)args;
	task_info_t *task_info = (task_info_t *)((uint8 *)os_get_back_from_list(&(p_sem->wait_task_list)) - LIST_NODE_ADDR_OFFSET);
	task_info->event_status = EVENT_WAIT_TIMEOUT;
	task_info->task_status = TASK_RUNNING;
	os_insert_runtree(task_info);
}

void os_sem_pend(os_sem_t *p_sem, os_size_t timeout, os_size_t *p_status)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	if (p_sem->sem > 0)
	{
		p_sem->sem--;
		if (p_status != NULL)
		{
			*p_status = EVENT_GET_SIGNAL;
		}
	}
	else
	{
		os_set_timer(&_running_task->timer, timeout, task_wakeup, p_sem);
		os_insert_to_front(&(p_sem->wait_task_list), &_running_task->list_node_structrue);
		_running_task->sem = p_sem;
		_running_task->task_status = TASK_WAIT;
		os_sw_out();
		os_close_timer(&_running_task->timer);
		if (p_status != NULL)
		{
			*p_status = _running_task->event_status;
		}
		_running_task->event_status = EVENT_NONE;
	}
	os_cpu_sr_restore(cpu_sr);
}

os_size_t os_sem_post(os_sem_t *p_sem)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	if (p_sem != NULL)
	{
		if (p_sem->wait_task_list != NULL)
		{
			task_info_t *task_info = (task_info_t *)((uint8 *)os_get_back_from_list(&(p_sem->wait_task_list)) - LIST_NODE_ADDR_OFFSET);
			os_remove_from_list(&(p_sem->wait_task_list), &task_info->list_node_structrue);
			task_info->event_status = EVENT_GET_SIGNAL;
			task_info->task_status = TASK_RUNNING;
			os_sw_in(task_info);
		}
		else
		{
			p_sem->sem++;
		}
	}
	os_cpu_sr_restore(cpu_sr);
	return p_sem->sem;
}

os_sem_t *os_sem_find(const char *name)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	os_sem_t *sem = (os_sem_t *)find_node(_os_sem.tree, (void *)name, find_compare, NULL);
	os_cpu_sr_restore(cpu_sr);
	return sem;
}

void os_sem_free(os_sem_t *p_sem)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	while (p_sem->wait_task_list != NULL)
	{
		while (p_sem->wait_task_list != NULL)
		{
			task_info_t *task_info = (task_info_t *)((uint8 *)os_get_back_from_list(&(p_sem->wait_task_list)) - LIST_NODE_ADDR_OFFSET);
			os_remove_from_list(&(p_sem->wait_task_list), &task_info->list_node_structrue);
			os_close_timer(&task_info->timer);
			task_info->event_status = EVENT_NONE;
			task_info->task_status = TASK_RUNNING;
			os_insert_runtree(task_info);
		}
	}
	if (p_sem->name[0] != '\0')
	{
		os_delete_node(&_os_sem.tree, &p_sem->tree);
	}
	os_kfree(p_sem);
	os_cpu_sr_restore(cpu_sr);
}

void os_remove_task_from_sem(os_sem_t *p_sem, list_node_type_def *node)
{
	os_remove_from_list(&(p_sem->wait_task_list), node);
}
