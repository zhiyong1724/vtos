#include "uart.h"
void uart_handler()
{
	if (rSUBSRCPND & 1)
	{
		//recv
		uart_send_byte(rURXH0);
		rSUBSRCPND |= 1;
	}
	if (rSUBSRCPND & (1 << 1))
	{
		//send
		rSUBSRCPND |= 1 << 1;
	}
	if (rSUBSRCPND & (1 << 2))
	{
		//err
		rSUBSRCPND |= 1 << 2;
	}

	rSRCPND |= 1 << 28;
	rINTPND |= 1 << 28;
}

void uart_init()
{
	rGPHCON &= ~(3 << 4 | 3 << 6);
	rGPHCON |= 2 << 4 | 2 << 6;
	rULCON0 &= ~(0x3); /* 8 bits */
	rULCON0 |= 0x3;
	rULCON0 &= ~(1 << 2); /* 1 stopbit */
	rULCON0 &= ~(1 << 5); /* no parity */
	rULCON0 &= ~(1 << 6); /* normal (not infrared) */

	rUCON0 &= ~(3); /* open send mode */
	rUCON0 |= 0x1;
	rUCON0 &= ~(3 << 2); /* open recv mode */
	rUCON0 |= 1 << 2;

	rUCON0 &= ~(3 << 10); /* PCLK */

	rUBRDIV0 = 26; /* baud rate = 115200 */

	rINTMSK &= ~(1<<28);
	rINTSUBMSK &= ~(1<<0);
}

void uart_set_baud(U32 baud)
{
	if (baud == 115200)
	{
		rUBRDIV0 = 26;
	}
	else if (baud == 9600)
	{
		rUBRDIV0 = 325;
	}
	else
	{
	}
}

/* send a byte and wait until sent. */
void uart_send_byte(U8 c)
{
	rUTXH0 = c;
	while (0 == (rUTRSTAT0 & (1 << 2)))
		;
}

void uart_send_str(char *msg)
{
	while (*msg)
	{
		uart_send_byte((U8)(*msg++));
	}
}

