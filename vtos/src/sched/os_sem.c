#include "sched/os_sem.h"
#include "vtos.h"
#include "sched/os_sched.h"
#include "base/os_string.h"
static struct os_sem _os_sem;
static os_size_t insert_compare(void *key1, void *key2, void *arg)
{
	os_sem_t *info1 = (os_sem_t *)key1;
	os_sem_t *info2 = (os_sem_t *)key2;
	os_size_t results = os_str_cmp(info1->name, info2->name);
	return results;
}

static os_size_t find_compare(void *key1, void *key2, void *arg)
{
	char *name = (char *)key1;
	os_sem_t *info = (os_sem_t *)key2;
	os_size_t results = os_str_cmp(name, info->name);
	return results;
}

void os_sem_init()
{
	_os_sem.tree = NULL;
}

os_sem_t *os_sem_create(os_sem_t *p_sem, os_size_t cnt, const char *name)
{
	if (name != NULL && name[0] != '\0')
	{
		p_sem->sem = cnt;
		p_sem->wait_task_list = NULL;
		os_str_cpy(p_sem->name, name, SEM_NAME_SIZE);
		if (os_insert_node(&_os_sem.tree, &p_sem->tree, insert_compare, NULL) != 0)
		{
			p_sem = NULL;
		}
		return p_sem;
	}
	return NULL;
}

static void task_wakeup(void *args)
{
	os_sem_t *p_sem = (os_sem_t *)args;
	task_info_t *task_info = (task_info_t *)((uint8 *)os_get_back_from_list(&(p_sem->wait_task_list)) - LIST_NODE_ADDR_OFFSET);
	os_remove_from_list(&(p_sem->wait_task_list), &task_info->list_node_structrue);
	task_info->event_status = EVENT_WAIT_TIMEOUT;
	os_resume_thread(task_info->pid);
}

void os_sem_pend(os_sem_t *p_sem, os_size_t timeout, os_size_t *p_status)
{
	if (p_sem->sem > 0)
	{
		p_sem->sem--;
		*p_status = EVENT_GET_SIGNAL;
	}
	else
	{
		os_set_timer(&p_sem->timer, timeout, task_wakeup, p_sem);
		os_insert_to_front(&(p_sem->wait_task_list), &_running_task->list_node_structrue);
		os_task_sleep();
		os_close_timer(&p_sem->timer);
		*p_status = _running_task->event_status;
		_running_task->event_status = EVENT_NONE;
	}
}

os_size_t os_sem_post(os_sem_t *p_sem)
{
	if (p_sem != NULL)
	{
		if (p_sem->wait_task_list != NULL)
		{
			task_info_t *task_info = (task_info_t *)((uint8 *)os_get_back_from_list(&(p_sem->wait_task_list)) - LIST_NODE_ADDR_OFFSET);
			os_remove_from_list(&(p_sem->wait_task_list), &task_info->list_node_structrue);
			task_info->event_status = EVENT_GET_SIGNAL;
			os_resume_thread(task_info->pid);
		}
		else
		{
			p_sem->sem++;
		}
	}
	return p_sem->sem;
}

os_sem_t *os_sem_find(const char *name)
{
	os_sem_t *sem = (os_sem_t *)find_node(_os_sem.tree, (void *)name, find_compare, NULL);
	return sem;
}

void os_sem_free(os_sem_t *p_sem)
{
	while (p_sem->wait_task_list != NULL)
	{
		task_info_t *task_info = (task_info_t *)((uint8 *)os_get_back_from_list(&(p_sem->wait_task_list)) - LIST_NODE_ADDR_OFFSET);
		os_remove_from_list(&(p_sem->wait_task_list), &task_info->list_node_structrue);
		task_info->event_status = EVENT_NONE;
		os_resume_thread(task_info->pid);
	}
	os_delete_node(&_os_sem.tree, &p_sem->tree);
	os_kfree(p_sem);
}
