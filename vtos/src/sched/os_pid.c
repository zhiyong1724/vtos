#include "sched/os_pid.h"
#include "base/os_string.h"
#include "base/os_bitmap_index.h"
static uint8 _pid_bitmap[PID_BITMAP_SIZE];
void init_pid(void)
{
	os_mem_set(_pid_bitmap, 0, sizeof(_pid_bitmap));
}

uint32 pid_get(void)
{
	uint32 ret = 0;
	uint32 i;
	for (i = 0; i < PID_BITMAP_SIZE; i++)
	{
		ret = _bitmap_index[_pid_bitmap[i]];
		if (ret != 8)
		{
			uint8 mark = 0x80;
			mark >>= ret;
			_pid_bitmap[i] |= mark;
			ret = 8 * i + ret;
			break;
		}
	}
	if(PID_BITMAP_SIZE == i)
	{
		ret = MAX_PID_COUNT;
	}
	return ret;
}

void pid_put(uint32 pid)
{
	if (pid < MAX_PID_COUNT)
	{
		uint32 i = pid >> 3;
		uint32 j = pid % 8;
		uint8 mark = 0x80;
		mark >>= j;
		mark = ~mark;
		_pid_bitmap[i] &= mark;
	}
}

