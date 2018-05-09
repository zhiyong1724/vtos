#ifndef __OS_CF_H__
#define __OS_CF_H__

#define OS_TASK_SW_HOOK_EN      0          //任务切换HOOK开关
#define OS_TIME_TICK_HOOK_EN    0          //时钟滴答HOOK开关

#define TASK_STACK_SIZE       4096         //默认线程的栈大小
#define TASK_NAME_SIZE        32           //任务名称的大小,由于存在系统线程，最小不能小于16字节
#endif
