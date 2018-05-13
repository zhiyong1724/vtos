#include "os_sched.h"
#include "os_cfg.h"
#include "os_cpu.h"
#include "base/os_string.h"
#include "vtos.h"
#include "os_pid.h"
#include "os_sched.h"
#include "os_sem.h"
const os_size_t TREE_NODE_ADDR_OFFSET = (os_size_t)sizeof(os_stk *);     //task_info_t对象中tree_node_structrue地址减去该对象地址
const os_size_t LIST_NODE_ADDR_OFFSET = (os_size_t)sizeof(os_stk *) + (os_size_t)sizeof(tree_node_type_def);  //task_info_t对象中list_node_structrue地址减去该对象地址
task_info_t *_running_task;
task_info_t *_next_task;
static os_sem_t *_thread_free_sem;
struct os_sched _scheduler;
static const os_size_t prio_to_weight[40] =
{
	88761, 71755, 56483, 46273, 36291,
	29154, 23254, 18705, 14949, 11916,
	9548,  7620,  6100,  4904,  3906,
	3121,  2501,  1991,  1586,  1277,
	1024,  820,   655,   526,   423,
	335,   272,   215,   172,   137,
	110,   87,    70,    56,    45,
	36,    29,    23,    18,    15
};

static int8 vruntime_compare(void *key1, void *key2, void *arg)
{
	task_info_t *info1 = (task_info_t *)((uint8 *)key1 - TREE_NODE_ADDR_OFFSET);
	task_info_t *info2 = (task_info_t *)((uint8 *)key2 - TREE_NODE_ADDR_OFFSET);
	if (info1->vruntime - _scheduler.min_vruntime <= info2->vruntime - _scheduler.min_vruntime)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

static void init_task_info(void(*task)(void *p_arg), void *p_arg,
	const char *name, task_info_t *task_structrue, os_size_t stack_size)
{
	task_structrue->stack_head = (os_stk *)os_kmalloc(stack_size);
#if (OS_STK_GROWTH == 0)
	task_structrue->stack = os_task_stk_init(task, p_arg, task_structrue->stack_head);
	task_structrue->stack_end = task_structrue->stack_head + stack_size / sizeof(os_stk) - 2;
	task_structrue->stack_end[0] = 0x55;
	task_structrue->stack_end[1] = 0xaa;
#else
	task_structrue->stack = os_task_stk_init(task, p_arg, &(task_structrue->stack_head)[stack_size / sizeof(os_stk) - 1]);
	task_structrue->stack_end = task_structrue->stack_head;
	task_structrue->stack_end[0] = 0x55;
	task_structrue->stack_end[1] = 0xaa;
#endif
	task_structrue->list_node_structrue.pre_node = NULL;
	task_structrue->list_node_structrue.next_node = NULL;
	task_structrue->prio = 0;
	task_structrue->unit_vruntime = prio_to_weight[task_structrue->prio + 20];
	task_structrue->vruntime = _scheduler.min_vruntime;
	task_structrue->task_status = TASK_RUNNING;
	task_structrue->pid = pid_get();
	task_structrue->event_status = EVENT_NONE;
	task_structrue->sem = NULL;
	os_str_cpy(task_structrue->name, name, TASK_NAME_SIZE);
}

static void init_os_sched(void)
{
	_scheduler.run_task_tree = NULL;
	_scheduler.end_task_list = NULL;
	_scheduler.suspend_task_list = NULL;
	_scheduler.total_task_count = 0;
	_scheduler.activity_task_count = 0;
	_scheduler.min_vruntime = 0;
	_scheduler.running = 0;
	_scheduler.cpu_percent = 0;
	os_vector_init(&_scheduler.task_info_index, sizeof(void *));
}

static void idle_task(void *p_arg)
{
	os_set_prio(-20);
	_scheduler.running = 1;
	for (;;)
	{
		os_task_idle_hook();
	}
}

static os_size_t create_idle_task(void)
{
	os_size_t pid = os_kthread_create(idle_task, NULL, "System Idle");
	task_info_t **v = (task_info_t **)os_vector_at(&_scheduler.task_info_index, pid);
	_running_task = *v;
	return pid;
}

static void thread_free_task(void *p_arg)
{
	uint32 status = EVENT_NONE;
	_thread_free_sem = os_sem_create(0, "thread free task");
	os_set_prio(19);
	for (;;)
	{
		os_sem_pend(_thread_free_sem, 0, &status);
		while (_scheduler.end_task_list != NULL)
		{
			task_info_t *del_task = (task_info_t *)((uint8 *)_scheduler.end_task_list - LIST_NODE_ADDR_OFFSET);
			os_remove_from_list(&_scheduler.end_task_list, _scheduler.end_task_list);
			void **v = (task_info_t **)os_vector_at(&_scheduler.task_info_index, del_task->pid);
			*v = NULL;
			pid_put(del_task->pid);
			os_kfree(del_task->stack_head);
			os_kfree(del_task);
			_scheduler.total_task_count--;
		}
	}
}

static os_size_t create_thread_free_task(void)
{
	return os_kthread_create(thread_free_task, NULL, "System Thread Free");
}

void os_init_scheduler(void)
{
	_running_task = NULL;
	_next_task = NULL;
	init_pid();
	init_os_sched();
	create_idle_task();
	create_thread_free_task();
}

os_size_t os_delete_thread(os_size_t pid)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	if (pid < os_vector_size(&_scheduler.task_info_index))
	{
		task_info_t **v = (task_info_t **)os_vector_at(&_scheduler.task_info_index, pid);
		if (*v != NULL && (*v)->task_status != TASK_DELETE)
		{
			switch ((*v)->task_status)
			{
			case TASK_RUNNING:
			{
				os_delete_node(&_scheduler.run_task_tree, &(*v)->tree_node_structrue);
				break;
			}
			case TASK_SUSPEND:
			{
				os_remove_from_list(&_scheduler.suspend_task_list, &(*v)->list_node_structrue);
				break;
			}
			case TASK_WAIT:
			{
				os_remove_task_from_sem((os_sem_t *)(*v)->sem, &(*v)->list_node_structrue);
				break;
			}
			case TASK_SLEEP:
			{
				os_remove_task_from_timer(&(*v)->tree_node_structrue);
				break;
			}
			default:
				break;
			}
			os_insert_to_front(&_scheduler.end_task_list, &(*v)->list_node_structrue);
			(*v)->task_status = TASK_DELETE;
			os_sem_post(_thread_free_sem);
			if (pid == _running_task->pid)
			{
				os_thread_switch();
			}
			else
			{
				_next_task = (task_info_t *)((uint8 *)os_get_leftmost_node(&_scheduler.run_task_tree) - TREE_NODE_ADDR_OFFSET);
			}
			os_cpu_sr_restore(cpu_sr);
			return 0;
		}
	}
	os_cpu_sr_restore(cpu_sr);
	return 1;
}

void os_sys_tick(void)
{
	if (1 == _scheduler.running)
	{
		os_size_t cpu_sr = os_cpu_sr_off();
#ifdef __WINDOWS__
		HANDLE stop = _running_task->handle;
		SuspendThread(stop);
#endif // __WINDOWS__
#if (OS_TIME_TICK_HOOK_EN > 0)
		os_time_tick_hook();
#endif
		_running_task->vruntime += _running_task->unit_vruntime;
		if (0x55 == _running_task->stack_end[0] && 0xaa == _running_task->stack_end[1])
		{
			os_delete_node(&_scheduler.run_task_tree, &_running_task->tree_node_structrue);
			os_insert_node(&_scheduler.run_task_tree, &_running_task->tree_node_structrue, vruntime_compare, NULL);
		}
		else
		{
			os_error_msg("stack overflow", _running_task->pid);
			os_delete_thread(_running_task->pid);
		}
		os_timer_tick();
		_next_task = (task_info_t *)((uint8 *)os_get_leftmost_node(&_scheduler.run_task_tree) - TREE_NODE_ADDR_OFFSET);
		_scheduler.min_vruntime = _next_task->vruntime;
		if (0 == _next_task->pid)
		{
			if (_scheduler.cpu_percent > 0)
			{
				_scheduler.cpu_percent--;
			}
		}
		else if (_scheduler.cpu_percent < 100)
		{
			_scheduler.cpu_percent++;
		}
		os_cpu_sr_restore(cpu_sr);
		os_ctx_sw();
#ifdef __WINDOWS__
		ResumeThread(stop);
#endif // __WINDOWS__
	}
}

os_size_t os_kthread_create(void(*task)(void *p_arg), void *p_arg, const char *name)
{
	return os_kthread_createEX(task, p_arg, name, TASK_STACK_SIZE);
}

os_size_t os_kthread_createEX(void(*task)(void *p_arg), void *p_arg, const char *name, os_size_t stack_size)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	task_info_t *task_info = (task_info_t *)os_kmalloc(sizeof(task_info_t));
	init_task_info(task, p_arg, name, task_info, stack_size);
#ifdef __WINDOWS__
	task_info->handle = CreateThread(NULL, stack_size, (LPTHREAD_START_ROUTINE)task, p_arg, CREATE_SUSPENDED, NULL);
#endif
	if (task_info->pid >= os_vector_size(&_scheduler.task_info_index))
	{
		os_vector_push_back(&_scheduler.task_info_index, &task_info);
	}
	else
	{
		task_info_t **v = (task_info_t **)os_vector_at(&_scheduler.task_info_index, task_info->pid);
		*v = task_info;
	}
	os_insert_node(&_scheduler.run_task_tree, &task_info->tree_node_structrue, vruntime_compare, NULL);
	_scheduler.total_task_count++;
	os_task_create_hook();
	os_cpu_sr_restore(cpu_sr);
	return task_info->pid;
}

void os_task_return(void)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	os_task_return_hook();
	os_delete_thread(_running_task->pid);
	os_cpu_sr_restore(cpu_sr);
}

void os_set_prio(int8 prio)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	if (prio < 40)
	{
		_running_task->prio = prio;
		_running_task->unit_vruntime = prio_to_weight[_running_task->prio + 20];
	}
	os_cpu_sr_restore(cpu_sr);
}

os_size_t os_total_thread_count(void)
{
	return _scheduler.total_task_count;
}

os_size_t os_activity_thread_count(void)
{
	return _scheduler.activity_task_count;
}

void os_thread_switch()
{
	//由于无法知道该任务到底运行了多久，综合考虑，就当做运行了半个节拍的虚拟运行时间
	_running_task->vruntime += (_running_task->unit_vruntime >> 1);
	os_delete_node(&_scheduler.run_task_tree, &_running_task->tree_node_structrue);
	_next_task = (task_info_t *)((uint8 *)os_get_leftmost_node(&_scheduler.run_task_tree) - TREE_NODE_ADDR_OFFSET);
	_next_task->vruntime -= (_next_task->unit_vruntime >> 1);
	_scheduler.min_vruntime = _next_task->vruntime;
	os_size_t cpu_sr = os_cpu_sr_on();
	os_ctx_sw();
	os_cpu_sr_restore(cpu_sr);
}

os_size_t os_cpu_percent(void)
{
	return _scheduler.cpu_percent;
}

void os_open_scheduler()
{
	_scheduler.running = 1;
}

void os_close_scheduler()
{
	_scheduler.running = 0;
}

os_size_t os_suspend_thread(os_size_t pid)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	if (pid < os_vector_size(&_scheduler.task_info_index))
	{
		task_info_t **v = (task_info_t **)os_vector_at(&_scheduler.task_info_index, pid);
		if (*v != NULL)
		{
			if (pid == _running_task->pid)
			{
				os_thread_switch();
			}
			else
			{
				os_delete_node(&_scheduler.run_task_tree, &(*v)->tree_node_structrue);
			}
			(*v)->task_status = TASK_SUSPEND;
			os_cpu_sr_restore(cpu_sr);
			return 0;
		}
	}
	os_cpu_sr_restore(cpu_sr);
	return 1;
}

os_size_t os_resume_thread(os_size_t pid)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	if (pid < os_vector_size(&_scheduler.task_info_index))
	{
		task_info_t **v = (task_info_t **)os_vector_at(&_scheduler.task_info_index, pid);
		if (*v != NULL)
		{
			if ((*v)->vruntime - _scheduler.min_vruntime > prio_to_weight[0])
			{
				//防止时间溢出
				(*v)->vruntime = _scheduler.min_vruntime;
			}

			os_insert_node(&_scheduler.run_task_tree, &(*v)->tree_node_structrue, vruntime_compare, NULL);
			(*v)->task_status = TASK_RUNNING;
			os_cpu_sr_restore(cpu_sr);
			return 0;
		}
	}
	os_cpu_sr_restore(cpu_sr);
	return 1;
}



