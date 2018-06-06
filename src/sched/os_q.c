#include "vtos.h"
#include "sched/os_q.h"
#include "base/os_string.h"
static struct os_q _os_q;
void os_q_init()
{
	_os_q.tree = NULL;
}

void os_q_uninit()
{
	while (_os_q.tree != NULL)
	{
		os_q_t *q = (os_q_t *)_os_q.tree;
		os_sem_free(q->sem);
		os_delete_node(&_os_q.tree, _os_q.tree);
		os_deque_free(&q->queue);
		os_free(q);
	}
}

static int8 insert_compare(void *key1, void *key2, void *arg)
{
	os_q_t *info1 = (os_q_t *)key1;
	os_q_t *info2 = (os_q_t *)key2;
	int8 results = os_str_cmp(info1->name, info2->name);
	return results;
}

static int8 find_compare(void *key1, void *key2, void *arg)
{
	char *name = (char *)key1;
	os_q_t *info = (os_q_t *)key2;
	int8 results = os_str_cmp(name, info->name);
	return results;
}

os_q_t *os_q_create(os_size_t unit_size, const char *name)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	os_q_t *p_queue = (os_q_t *)os_malloc(sizeof(os_q_t));
	if (name != NULL && name[0] != '\0')
	{
		os_str_cpy(p_queue->name, name, SEM_NAME_SIZE);
		if (os_insert_node(&_os_q.tree, &p_queue->tree, insert_compare, NULL) != 0)
		{
			os_free(p_queue);
			p_queue = NULL;
		}
	}
	else
	{
		p_queue->name[0] = '\0';
	}
	if (p_queue != NULL)
	{
		p_queue->sem = os_sem_create(0, NULL);
		os_deque_init(&p_queue->queue, unit_size);
	}
	os_cpu_sr_restore(cpu_sr);
	return p_queue;
}

void os_q_reset(os_q_t *p_queue)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	os_deque_clear(&p_queue->queue);
	os_cpu_sr_restore(cpu_sr);
}

void *os_q_pend(os_q_t *p_queue, os_size_t timeout, os_size_t *p_status, void *p_out)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	void *ret = NULL;
	os_sem_pend(p_queue->sem, timeout, p_status);
	if (0 == timeout || EVENT_GET_SIGNAL == *p_status)
	{
		ret = os_deque_front(&p_queue->queue);
		os_mem_cpy(p_out, ret, p_queue->queue.unit_size);
		os_deque_pop_front(&p_queue->queue);
	}
	os_cpu_sr_restore(cpu_sr);
	return ret;
}

os_size_t os_q_post(os_q_t *p_queue, void *p_msg)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	os_deque_push_back(&p_queue->queue, p_msg);
	os_sem_post(p_queue->sem);
	os_cpu_sr_restore(cpu_sr);
	return (p_queue->sem->sem);
}

os_q_t *os_q_find(const char *name)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	os_q_t *q = (os_q_t *)find_node(_os_q.tree, (void *)name, find_compare, NULL);
	os_cpu_sr_restore(cpu_sr);
	return q;
}

void os_q_free(os_q_t *p_queue)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	if (p_queue->name[0] != '\0')
	{
		os_delete_node(&_os_q.tree, &p_queue->tree);
	}
	os_deque_free(&p_queue->queue);
	os_sem_free(p_queue->sem);
	os_free(p_queue);
	os_cpu_sr_restore(cpu_sr);
}
