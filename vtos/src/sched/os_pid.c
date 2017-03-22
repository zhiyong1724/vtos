#include "sched/os_pid.h"
#include "lib/os_string.h"
static uint8 _pid_bitmap[PID_BITMAP_SIZE];
static const uint8 _pid_bitmap_index[256] =
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         //0-15
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         //16-31
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         //32-47
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         //48-63
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         //64-79
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         //80-95
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         //96-111
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         //112-127
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,         //128-143
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,         //144-159
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,         //160-175
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,         //176-191
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,         //192-207
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,         //208-223
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,         //224-239
		4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8          //240-256
		};
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
		ret = _pid_bitmap_index[_pid_bitmap[i]];
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

