#ifndef _OS_CF_H
#define _OS_CF_H

#define OS_TASK_SW_HOOK_EN      0          //任务切换HOOK开关
#define OS_TIME_TICK_HOOK_EN    0          //时钟滴答HOOK开关

#define TASK_STACK_SIZE       2048         //默认线程的栈大小
#define MAX_PID_COUNT         64           //最多可以分配的PID数，也表示最大任务数
#define TASK_NAME_SIZE        32           //任务名称的大小,由于存在系统线程，最小不能小于16字节
#endif
