#include "sched/os_pid.h"
#include "base/os_string.h"
#include "base/os_bitmap_index.h"
static struct os_pid _os_pid;
void init_pid(void)
{
	os_vector_init(&_os_pid.pid_bitmap, 1);
}

void uninit_pid(void)
{
	os_vector_free(&_os_pid.pid_bitmap);
}

static void expand_bitmap()
{
	uint8 v = 0;
	os_vector_push_back(&_os_pid.pid_bitmap, &v);
}

os_size_t pid_get(void)
{
	os_size_t ret = 0;
	os_size_t i;
	os_size_t max_size = os_vector_size(&_os_pid.pid_bitmap);

	for (i = 0; i <= max_size; i++)
	{
		if (i == max_size)
		{
			expand_bitmap();
		}
		uint8 *v = os_vector_at(&_os_pid.pid_bitmap, i);
		ret = _bitmap_index[*v];
		if (ret != 8)
		{
			uint8 mark = 0x80;
			mark >>= ret;
			*v |= mark;
			ret = 8 * i + ret;
			break;
		}
	}
	return ret;
}

void pid_put(os_size_t pid)
{
	os_size_t max_size = os_vector_size(&_os_pid.pid_bitmap);
	if (pid < max_size * 8)
	{
		os_size_t i = pid >> 3;
		os_size_t j = pid % 8;
		uint8 mark = 0x80;
		mark >>= j;
		mark = ~mark;
		uint8 *v = os_vector_at(&_os_pid.pid_bitmap, i);
		*v &= mark;
	}
}

