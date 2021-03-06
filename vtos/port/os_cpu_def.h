#ifndef _OS_CPU_DEF_H
#define _OS_CPU_DEF_H
/*与编译器相关的类型定义*/
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long uint64;
typedef signed long int64;
typedef unsigned int os_stk; //堆栈类型，堆栈存取宽度
typedef unsigned int os_cpu_sr; //用于保存CPU状态寄存器类型
typedef unsigned int os_size_t; //这个类型要保证能放下一个地址
#endif