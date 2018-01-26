#ifndef __OS_SCHED_DEF_H__
#define __OS_SCHED_DEF_H__
enum task_status_def
{
	TASK_RUNNING,
	TASK_SLEEP,
};

enum event_status_def
{
	EVENT_NONE,
	EVENT_GET_SIGNAL,
	EVENT_WAIT_TIMEOUT
};
#endif
