#include "led.h"
#include "uart.h"
#include "timer.h"
#include "vtos.h"
static void task_a(void *p_arg)
{
	static int i = 0;
	for(;;)
	{
		if(0 == i)
		{
			i = 1;
			led_on(LED_1);
		}
		else
		{
			i = 0;
			led_off(LED_1);
		}
		os_sleep(100);
	}
}

static void task_b(void *p_arg)
{
	static int i = 0;
	for (;;)
	{
		if (0 == i)
		{
			i = 1;
			led_on(LED_2);
		}
		else
		{
			i = 0;
			led_off(LED_2);
		}
		os_sleep(300);
	}
}

static void task_c(void *p_arg)
{
	static int i = 0;
	for (;;)
	{
		if (0 == i)
		{
			i = 1;
			led_on(LED_3);
		}
		else
		{
			i = 0;
			led_off(LED_3);
		}
		os_sleep(500);
	}
}

static void task_d(void *p_arg)
{
	static int i = 0;
	for (;;)
	{
		if (0 == i)
		{
			i = 1;
			led_on(LED_4);
		}
		else
		{
			i = 0;
			led_off(LED_4);
		}
		os_sleep(700);
	}
}

int main()
{
	led_init();
	timer4_init();
	uart_init();
	if(0 == os_sys_init())
	{
		os_kthread_create(task_a, NULL, "task_a");
		os_kthread_create(task_b, NULL, "task_b");
		os_kthread_create(task_c, NULL, "task_c");
		os_kthread_create(task_d, NULL, "task_c");
		os_sys_start();
	}
    return 0;    
}


