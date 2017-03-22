#include "uart.h"
#include "nandflash.h"
#include "timer.h"
#define TEXT_SIZE 0x200000  //2M
#define START_ADDR 0x30000000
#define INTOFFSET (*(volatile unsigned int *)0x4a000014)
void irq_handler()
{
	switch (INTOFFSET)
	{
	case 14:
	{
		//INT_TIMER4
		timer4_handler();
		break;
	}
	case 28:
	{
		//INT_UART0
		uart_handler();
		break;
	}
	}
}

/*void swi_handler()
{
}*/

void load_code()
{
	unsigned int n = TEXT_SIZE / NF_PAGE_SIZE;
	int i = 0;
	unsigned char *pstart = (unsigned char *)START_ADDR;
	NF_Init();
	for(i = 0; i < n; i++)
	{
		rNF_ReadPage(i, (void *)pstart);
		pstart += NF_PAGE_SIZE;
	}
}
