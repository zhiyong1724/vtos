#include "timer.h"
#include "vtos.h"
void timer4_handler()
{
	rSRCPND |= (1 << 14);
	rINTPND |= (1 << 14);
	os_sys_tick();
}
//一毫秒触发一次中断
void timer4_init()
{
	rTCFG0 &= ~(0xff << 8);
	rTCFG0 |= (249 << 8);
	rTCFG1 &= ~(0x0f << 16);
	rTCNTB4 = 1000;
	rTCON |= (1 << 21);//填充值
	rTCON |= (1 << 22);//自动装载
	rINTMSK &= ~(1 << 14);
	rTCON &= ~(1 << 21);
	rTCON |= (1 << 20);//启动定时器
}
