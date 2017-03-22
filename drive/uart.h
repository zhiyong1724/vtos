#ifndef _UART_H
#define _UART_H
#include "def.h"
void uart_handler();
void uart_init();
void uart_set_baud(U32 baud);
void uart_send_byte(U8 c);
void uart_send_str(char *msg);
#endif
