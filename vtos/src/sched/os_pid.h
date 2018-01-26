#ifndef __OS_PID_H__
#define __OS_PID_H__
#include "os_cpu.h"
#define PID_BITMAP_SIZE        (MAX_PID_COUNT / 8)
void init_pid(void);
uint32 pid_get(void);
void pid_put(uint32 pid);
#endif
