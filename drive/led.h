#ifndef _LED_H
#define _LED_H
#include "def.h"
enum LED_TYPE
{
	LED_1,
	LED_2,
	LED_3,
	LED_4
};
void led_init();
void led_on(int led_type);
void led_off(int led_type);
#endif
