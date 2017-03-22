#include "led.h"
void led_init()
{
	GPBCON &= 0xfffd57ff;
	GPBCON |= 0x00015400;
	GPBDAT |= 0x0f << 5;
}

void led_on(int led_type)
{
	switch(led_type)
	{
	case LED_1:
	{
		GPBDAT &= 0xffffffdf;
		break;
	}
	case LED_2:
	{
		GPBDAT &= 0xffffffbf;
		break;
	}
	case LED_3:
	{
		GPBDAT &= 0xffffff7f;
		break;
	}
	case LED_4:
	{
		GPBDAT &= 0xfffffeff;
		break;
	}
	}
}
void led_off(int led_type)
{
	switch (led_type)
	{
	case LED_1:
	{
		GPBDAT |= 0x00000020;
		break;
	}
	case LED_2:
	{
		GPBDAT |= 0x00000040;
		break;
	}
	case LED_3:
	{
		GPBDAT |= 0x00000080;
		break;
	}
	case LED_4:
	{
		GPBDAT |= 0x00000100;
		break;
	}
	}
}
