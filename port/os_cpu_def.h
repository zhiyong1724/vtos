#ifndef __OS_CPU_DEF_H__
#define __OS_CPU_DEF_H__
#ifdef __WINDOWS__
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#endif
#ifndef NULL
#define NULL 0
#endif
#ifdef __cplusplus
#define VTOS_API extern "C"
#else
#define VTOS_API
#endif
/*与编译器相关的类型定义*/
#ifdef __CPU64__
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;
typedef unsigned long long os_stk; //堆栈类型，堆栈存取宽度
typedef unsigned long long os_cpu_sr; //用于保存CPU状态寄存器类型
typedef unsigned long long os_size_t; //这个类型要保证能放下一个地址
#else
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;
typedef unsigned int os_stk; //堆栈类型，堆栈存取宽度
typedef unsigned int os_cpu_sr; //用于保存CPU状态寄存器类型
typedef unsigned int os_size_t; //这个类型要保证能放下一个地址
#endif
#endif