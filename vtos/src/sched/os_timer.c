#include "vtos.h"
#include "sched/os_sched.h"
#include "sched/os_timer.h"
#include "vtos.h"
static struct timer_controler_t _timer_controler;
void os_init_timer(void)
{
	_timer_controler.timer_tree = NULL;
	_timer_controler.tick = 0;
	_timer_controler.min_timer = NULL;
}

static void os_insert_to_timer_tree(tree_node_type_def **handle, timer_info_t *timer_info)
{
	tree_node_type_def *cur_node = *handle;
	os_init_node(&(timer_info->tree_node_structrue));
	if (NULL == *handle)
	{
		timer_info->tree_node_structrue.color = BLACK;
		*handle = &(timer_info->tree_node_structrue);
	}
	else
	{
		for (;;)
		{
			if (timer_info->tick - _timer_controler.tick <= ((timer_info_t *)cur_node)->tick - _timer_controler.tick)
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
		timer_info->tree_node_structrue.parent = cur_node;
		if (timer_info->tick - _timer_controler.tick <= ((timer_info_t *)cur_node)->tick - _timer_controler.tick)
			cur_node->left_tree = &(timer_info->tree_node_structrue);
		else
			cur_node->right_tree = &(timer_info->tree_node_structrue);
		os_insert_case(handle, &(timer_info->tree_node_structrue));
	}
}

static void task_wakeup(void *args)
{
	uint32 pid = *(uint32 *)args;
	os_task_activity(pid);
}

void os_sleep(uint64 t)
{
	os_cpu_sr cpu_sr = os_cpu_sr_off();
	os_set_timer(&_running_task->timer, t, task_wakeup, &(_running_task->pid));
	os_cpu_sr_restore(cpu_sr);
	os_task_sleep();
}

void os_close_timer(timer_info_t *timer_info)
{
	if (timer_info->tick > 0 && _timer_controler.timer_tree != NULL)
	{
		os_delete_node(&_timer_controler.timer_tree, &timer_info->tree_node_structrue);
		timer_info->tick = 0;
		_timer_controler.min_timer = (timer_info_t *)os_get_leftmost_node(&_timer_controler.timer_tree);
	}
}

void os_timer_tick(void)
{
	_timer_controler.tick++;
	while (_timer_controler.min_timer != NULL && _timer_controler.min_timer->tick == _timer_controler.tick)
	{
		(_timer_controler.min_timer->call_back_func)(_timer_controler.min_timer->args);
		os_delete_node(&_timer_controler.timer_tree, &_timer_controler.min_timer->tree_node_structrue);
		_timer_controler.min_timer = (timer_info_t *)os_get_leftmost_node(&_timer_controler.timer_tree);
	}
}

timer_info_t *os_set_timer(timer_info_t *timer, uint64 time, timer_call_back call_back_func, void *args)
{
	uint64 tick = time / (TICK_TIME / 1000);
	if (tick > 0)
	{
		timer->tick = _timer_controler.tick + tick;
		timer->call_back_func = call_back_func;
		timer->args = args;
		os_insert_to_timer_tree(&_timer_controler.timer_tree, timer);
		_timer_controler.min_timer = (timer_info_t *)os_get_leftmost_node(&_timer_controler.timer_tree);
	}
	else
	{
		timer->tick = 0;
	}
	return timer;
}

