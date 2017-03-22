#include "led.h"
#include "uart.h"
#include "timer.h"
#include "vtos.h"
#include "lib/os_string.h"
void *_message_queue[16];
os_q_t _handle;
static void task_a(void *p_arg);
static void task_b(void *p_arg);
static void task_c(void *p_arg);
static void task_d(void *p_arg);
static void task_a(void *p_arg)
{
	char buff[32];
	static int i = 0;
	for(;;)
	{
		if(0 == i)
		{
			i = 1;
		}
		else
		{
			i = 0;
		}
		os_q_post(&_handle, &i);
		os_utoa(buff, os_used_mem_size());
		uart_send_str(buff);
		uart_send_str("\n");
		os_sleep(500);
	}
}

static void task_b(void *p_arg)
{
	uint32 status = 0;
	for(;;)
	{
		int *p = (int *)os_q_pend(&_handle, 0, &status);
		if(0 == *p)
		{
			led_off(LED_3);
		}
		else
		{
			led_on(LED_3);
		}
	}
}

static void task_c(void *p_arg)
{
	static int i = 0;
	os_sem_t handle;
	uint32 status;
	os_sem_create(&handle, 0);
	for(;;)
	{
		if (0 == i)
		{
			i = 1;
		}
		else
		{
			i = 0;
		}
		os_kthread_create(task_d, &i, "task_d");
		os_sem_pend(&handle, 700, &status);
	}
}

static void task_d(void *p_arg)
{
	int *p = p_arg;
	if (0 == *p)
	{
		led_off(LED_4);
	}
	else
	{
		led_on(LED_4);
	}
}

int main()
{
	led_init();
	timer4_init();
	uart_init();
	if(0 == os_sys_init())
	{
		os_q_create(&_handle, _message_queue, 16);
		os_kthread_create(task_a, NULL, "task_a");
		os_kthread_create(task_b, NULL, "task_b");
		os_kthread_create(task_c, NULL, "task_c");
		os_sys_start();
	}
    return 0;    
}


