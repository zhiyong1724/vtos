#include "vtos.h"
#include "sched/os_sched.h"
#include "sched/os_timer.h"
#include "vtos.h"
static struct os_timer _timer_controler;
void os_init_timer(void)
{
	_timer_controler.timer_tree = NULL;
	_timer_controler.tick = 0;
	_timer_controler.min_timer = NULL;
}

static int8 time_compare(void *key1, void *key2, void *arg)
{
	timer_info_t *info1 = (timer_info_t *)key1;
	timer_info_t *info2 = (timer_info_t *)key2;
	if (info1->tick - _timer_controler.tick <= info2->tick - _timer_controler.tick)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

static void task_wakeup(void *args)
{
	os_size_t pid = *(os_size_t *)args;
	os_resume_thread(pid);
}

void os_sleep(os_size_t t)
{
	os_size_t cpu_sr = os_cpu_sr_off();
	os_set_timer(&_running_task->timer, t, task_wakeup, &(_running_task->pid));
	_running_task->task_status = TASK_SLEEP;
	os_thread_switch();
	os_cpu_sr_restore(cpu_sr);
}

void os_remove_task_from_timer(tree_node_type_def *node)
{
	os_delete_node(&_timer_controler.timer_tree, node);
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

timer_info_t *os_set_timer(timer_info_t *timer, os_size_t time, timer_call_back call_back_func, void *args)
{
	os_size_t tick = time / (TICK_TIME / 1000);
	if (tick > 0)
	{
		timer->tick = _timer_controler.tick + tick;
		timer->call_back_func = call_back_func;
		timer->args = args;
		os_insert_node(&_timer_controler.timer_tree, &timer->tree_node_structrue, time_compare, NULL);
		_timer_controler.min_timer = (timer_info_t *)os_get_leftmost_node(&_timer_controler.timer_tree);
	}
	else
	{
		timer->tick = 0;
	}
	return timer;
}

