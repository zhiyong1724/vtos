#include "os_sched.h"
#include "os_cfg.h"
#include "os_cpu.h"
#include "lib/os_string.h"
#include "vtos.h"
#include "os_pid.h"
#include "os_sched_def.h"
#include "os_sem.h"

const os_size_t TREE_NODE_ADDR_OFFSET = (os_size_t)sizeof(os_stk *);     //task_info_t对象中tree_node_structrue地址减去该对象地址
const os_size_t LIST_NODE_ADDR_OFFSET = (os_size_t)sizeof(os_stk *) + (os_size_t)sizeof(tree_node_type_def);  //task_info_t对象中list_node_structrue地址减去该对象地址
task_info_t *_running_task;
task_info_t *_next_task;
static os_sem_t _thread_release_sem;
struct scheduler_info _scheduler;
static const uint32 prio_to_weight[40] =
{ 88761, 71755, 56483, 46273, 36291, //
		29154, 23254, 18705, 14949, 11916, //
		9548, 7620, 6100, 4904, 3906, //
		3121, 2501, 1991, 1586, 1277, //
		1024, 820, 655, 526, 423, //
		335, 272, 215, 172, 137, //
		110, 87, 70, 56, 45, //
		36, 29, 23, 18, 15 //
		};
static void os_insert_to_run_task_tree(tree_node_type_def **handle, task_info_t *task_structrue)
{
	tree_node_type_def *cur_node = *handle;
	task_info_t *cur_task_structrue = NULL;
	os_init_node(&(task_structrue->tree_node_structrue));
	if (NULL == *handle)
	{
		task_structrue->tree_node_structrue.color = BLACK;
		*handle = &(task_structrue->tree_node_structrue);
	}
	else
	{
		for (;;)
		{
			cur_task_structrue = (task_info_t *)((uint8 *)cur_node
				- TREE_NODE_ADDR_OFFSET);
			if (task_structrue->vruntime - _scheduler.min_vruntime <= cur_task_structrue->vruntime - _scheduler.min_vruntime)
			{
				if (cur_node->left_tree == &_leaf_node)
				{
					break;
				}
				cur_node = cur_node->left_tree;
			}
			else
			{
				if (cur_node->right_tree == &_leaf_node)
				{
					break;
				}
				cur_node = cur_node->right_tree;
			}
		}
		task_structrue->tree_node_structrue.parent = cur_node;
		cur_task_structrue = (task_info_t *)((uint8 *)cur_node
			- TREE_NODE_ADDR_OFFSET);
		if (task_structrue->vruntime - _scheduler.min_vruntime <= cur_task_structrue->vruntime - _scheduler.min_vruntime)
			cur_node->left_tree = &(task_structrue->tree_node_structrue);
		else
			cur_node->right_tree = &(task_structrue->tree_node_structrue);
		os_insert_case(handle, &(task_structrue->tree_node_structrue));
	}
	_scheduler.activity_task_count++;
}

static void os_remove_from_run_task_tree(tree_node_type_def **handle, task_info_t *task_structrue)
{
	os_delete_node(handle, &(task_structrue->tree_node_structrue));
	_scheduler.activity_task_count--;
}

static void init_task_structrue(void (*task)(void *p_arg), void *p_arg,
		const char *name, task_info_t *task_structrue, os_size_t stack_size)
{
	task_structrue->stack_head = (os_stk *) os_kmalloc(stack_size);
	if (NULL == task_structrue->stack_head)
	{
		return;
	}
#if (OS_STK_GROWTH == 0)
	task_structrue->stack = os_task_stk_init(task, p_arg,
			task_structrue->stack_head);
	task_structrue->stack_end = task_structrue->stack_head + stack_size - 2;
	task_structrue->stack_end[0] = 0x55;
	task_structrue->stack_end[1] = 0xaa;
#else
	task_structrue->stack =
			os_task_stk_init(task, p_arg,
					&(task_structrue->stack_head)[stack_size
							/ sizeof(os_stk) - 1]);
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
	os_str_cpy(task_structrue->name, name, TASK_NAME_SIZE);
}

static void init_scheduler_structure(void)
{
	_scheduler.run_task_tree = NULL;
	_scheduler.end_task_list = NULL;
	_scheduler.total_task_count = 0;
	_scheduler.activity_task_count = 0;
	_scheduler.min_vruntime = 0;
	os_mem_set(_scheduler.task_info_index, 0, sizeof(_scheduler.task_info_index));
	_scheduler.running = 0;
}

static void idle_task(void *p_arg)
{
	os_set_prio(-20);
	_scheduler.running = 1;
	for (;;)
	{
		os_cpu_sr cpu_sr = os_cpu_sr_save();
		os_task_idle_hook();
		_running_task->vruntime += _running_task->unit_vruntime;
		os_remove_from_run_task_tree(&_scheduler.run_task_tree, _running_task);
		os_insert_to_run_task_tree(&_scheduler.run_task_tree, _running_task);
		_next_task = (task_info_t *) ((uint8 *) os_get_leftmost_node(
				&_scheduler.run_task_tree) - TREE_NODE_ADDR_OFFSET);
		_scheduler.min_vruntime = _next_task->vruntime;
		os_cpu_sr_restore(cpu_sr);
		os_ctx_sw();
	}
}

static os_size_t create_idle_task(void)
{
	os_size_t ret = 1;
	task_info_t *task_structrue = (task_info_t *) os_kmalloc(
			sizeof(task_info_t));
	if (task_structrue != NULL)
	{
		init_task_structrue(idle_task, NULL, "IdleTask", task_structrue, TASK_STACK_SIZE);
		if (task_structrue->stack_head != NULL
				&& task_structrue->pid < MAX_PID_COUNT)
		{
			_scheduler.task_info_index[task_structrue->pid] = task_structrue;
			os_insert_to_run_task_tree(&_scheduler.run_task_tree, task_structrue);
			_next_task = task_structrue;
			_scheduler.total_task_count++;
			ret = 0;
		}
		else if (task_structrue->stack_head != NULL)
		{
			os_kfree(task_structrue->stack_head);
			os_kfree(task_structrue);
		}
		else
		{
			os_kfree(task_structrue);
		}
	}
	return ret;
}

static void thread_release_task(void *p_arg)
{
	uint32 status = EVENT_NONE;
	os_cpu_sr cpu_sr;
	os_set_prio(-20);
	for (;;)
	{
		os_sem_pend(&_thread_release_sem, 0, &status);
		cpu_sr = os_cpu_sr_save();
		while (_scheduler.end_task_list != NULL )
		{
			task_info_t *del_task = (task_info_t *)((uint8 *)_scheduler.end_task_list - LIST_NODE_ADDR_OFFSET);
			os_remove_from_list(&_scheduler.end_task_list, _scheduler.end_task_list);
			_scheduler.task_info_index[del_task->pid] = NULL;
			pid_put(del_task->pid);
			os_kfree(del_task->stack_head);
			os_kfree(del_task);
			_scheduler.total_task_count--;
		}
		os_cpu_sr_restore(cpu_sr);
	}
}

static os_size_t create_thread_release_task(void)
{
	os_size_t ret = 1;
	os_sem_create(&_thread_release_sem, 0);
	ret = os_kthread_create(thread_release_task, NULL, "ThreadRelease");
	return ret;
}

os_size_t os_init_scheduler(void)
{
	os_size_t ret = 1;
	_running_task = NULL;
	_next_task = NULL;
	init_pid();
	init_scheduler_structure();
	ret = create_idle_task();
	ret = create_thread_release_task();
	return ret;
}

static void do_exit(void)
{
	os_cpu_sr cpu_sr = os_cpu_sr_save();
	os_remove_from_run_task_tree(&_scheduler.run_task_tree, _running_task);
	os_insert_to_front(&_scheduler.end_task_list, &_running_task->list_node_structrue);
	_next_task = (task_info_t *)((uint8 *)os_get_leftmost_node(&_scheduler.run_task_tree)
		- TREE_NODE_ADDR_OFFSET);

	os_sem_post(&_thread_release_sem);
	os_cpu_sr_restore(cpu_sr);
	os_ctx_sw();
}

void os_sys_tick(void)
{
	os_cpu_sr cpu_sr = os_cpu_sr_save();
#ifdef __WINDOWS__
	if (0 == _scheduler.running)
	{
#else
	if (1 == _scheduler.running)
	{
#endif
#if (OS_TIME_TICK_HOOK_EN > 0)
	os_time_tick_hook ();
#endif
		_running_task->vruntime += _running_task->unit_vruntime;
		if(0x55 == _running_task->stack_end[0] && 0xaa == _running_task->stack_end[1])
		{
			os_remove_from_run_task_tree(&_scheduler.run_task_tree, _running_task);
			os_insert_to_run_task_tree(&_scheduler.run_task_tree, _running_task);
		}
		else
		{
			os_error_msg("stack overflow", _running_task->pid);
			do_exit();
		}
		_next_task = (task_info_t *)((uint8 *)os_get_leftmost_node(&_scheduler.run_task_tree)
				- TREE_NODE_ADDR_OFFSET);
		_scheduler.min_vruntime = _next_task->vruntime;
		os_timer_tick();
		os_cpu_sr_restore(cpu_sr);
		os_int_ctx_sw();
	}
	os_cpu_sr_restore(cpu_sr);
}

os_size_t os_kthread_create(void (*task)(void *p_arg), void *p_arg, const char *name)
{
	os_size_t ret = 1;
	os_cpu_sr cpu_sr = os_cpu_sr_save();
	task_info_t *task_structrue = (task_info_t *) os_kmalloc(
			sizeof(task_info_t));
	if (task_structrue != NULL)
	{
		init_task_structrue(task, p_arg, name, task_structrue, TASK_STACK_SIZE);
		if (task_structrue->stack_head != NULL
				&& task_structrue->pid < MAX_PID_COUNT)
		{
			_scheduler.task_info_index[task_structrue->pid] = task_structrue;
			os_insert_to_run_task_tree(&_scheduler.run_task_tree, task_structrue);
			_scheduler.total_task_count++;
			ret = 0;
		}
		else if (task_structrue->stack_head != NULL)
		{
			os_kfree(task_structrue->stack_head);
			os_kfree(task_structrue);
		}
		else
		{
			os_kfree(task_structrue);
		}
	}
	os_task_create_hook();
	os_cpu_sr_restore(cpu_sr);
	return ret;
}

os_size_t os_kthread_createEX(void (*task)(void *p_arg), void *p_arg, const char *name, os_size_t stack_size)
{
	os_size_t ret = 1;
	os_cpu_sr cpu_sr = os_cpu_sr_save();
	task_info_t *task_structrue = (task_info_t *) os_kmalloc(
			sizeof(task_info_t));
	if (task_structrue != NULL)
	{
		init_task_structrue(task, p_arg, name, task_structrue, stack_size);
		if (task_structrue->stack_head != NULL
				&& task_structrue->pid < MAX_PID_COUNT)
		{
			_scheduler.task_info_index[task_structrue->pid] = task_structrue;
			os_insert_to_run_task_tree(&_scheduler.run_task_tree,
					task_structrue);
			_scheduler.total_task_count++;
			ret = 0;
		}
		else if (task_structrue->stack_head != NULL)
		{
			os_kfree(task_structrue->stack_head);
			os_kfree(task_structrue);
		}
		else
		{
			os_kfree(task_structrue);
		}
	}
	os_task_create_hook();
	os_cpu_sr_restore(cpu_sr);
	return ret;
}

void os_task_return(void)
{
	os_task_return_hook();
	do_exit();
}

void os_set_prio(int32 prio)
{
	if (prio >= -20 && prio < 20)
	{
		os_cpu_sr cpu_sr = os_cpu_sr_save();
		_running_task->prio = prio;
		_running_task->unit_vruntime = prio_to_weight[_running_task->prio + 20];
		os_cpu_sr_restore(cpu_sr);
	}
}

os_size_t os_total_thread_count(void)
{
	return _scheduler.total_task_count;
}

os_size_t os_activity_thread_count(void)
{
	return _scheduler.activity_task_count;
}

void os_task_sleep()
{
	os_cpu_sr cpu_sr = os_cpu_sr_save();
	_running_task->task_status = TASK_SLEEP;
	//由于无法知道该任务到底运行了多久，综合考虑，就当做运行了半个节拍的虚拟运行时间
	_running_task->vruntime += (_running_task->unit_vruntime >> 1);
	os_remove_from_run_task_tree(&_scheduler.run_task_tree, _running_task);
	_next_task = (task_info_t *) ((uint8 *) os_get_leftmost_node(
			&_scheduler.run_task_tree) - TREE_NODE_ADDR_OFFSET);
	_next_task->vruntime -= (_next_task->unit_vruntime >> 1);
	_scheduler.min_vruntime = _next_task->vruntime;
	os_cpu_sr_restore(cpu_sr);
	os_ctx_sw();
}

void os_task_activity(uint32 pid)
{
	if (_scheduler.task_info_index[pid] != NULL)
	{
		if (_scheduler.task_info_index[pid]->vruntime - _scheduler.min_vruntime > prio_to_weight[0])
		{
			//防止时间溢出
			_scheduler.task_info_index[pid]->vruntime = _scheduler.min_vruntime;
		}

		os_insert_to_run_task_tree(&_scheduler.run_task_tree, _scheduler.task_info_index[pid]);
	}
}
