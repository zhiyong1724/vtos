﻿#ifndef __OS_STRIN_H__
#define __OS_STRIN_H__
#include "os_cpu.h"
/*********************************************************************************************************************
 * 该函数可以设置一段内存空间的值
 * s：指向要被设置的内存空间
 * ch：需要被设置的值
 * n：要设置的大小
 * return：返回指向要被设置的内存空间
 *********************************************************************************************************************/
void *os_mem_set(void *s, int8 ch, os_size_t n);
/*********************************************************************************************************************
* 该函数可以拷贝一段内存空间的值
* dest：指向被拷贝内存的存放空间
* src：指向被拷贝内存
* n：拷贝大小
* return：返回dest指针
*********************************************************************************************************************/
void *os_mem_cpy(void *dest, void *src, os_size_t n);
/*********************************************************************************************************************
 * 字符串拷贝函数
 * dest：指向被拷贝字符串的存放空间
 * src：指向被拷贝字符串
 * n：最大的拷贝大小
 * return：返回指向被拷贝字符串空间
 *********************************************************************************************************************/
void *os_str_cpy(char *dest, const char *src, os_size_t n);
/*********************************************************************************************************************
 * 把一个无符号整数转换成字符串
 * buff：保存转换后的字符串
 * num：要转换的无符号整数
 *********************************************************************************************************************/
void os_utoa(char *buff, os_size_t num);
/*********************************************************************************************************************
* 把一个字符串数字转换成无符号整数
* buff：要转换的字符串
* return：转换后的的无符号整数
*********************************************************************************************************************/
os_size_t os_atou(const char *buff);
/*********************************************************************************************************************
 * 返回字符串长度
 * str：字符串
 * return：字符串长度
 *********************************************************************************************************************/
os_size_t os_str_len(const char *str);
/*********************************************************************************************************************
 * 匹配字符串
 * str：源字符串
 * pattern：模式字符串
 * return：-1：匹配失败；other：匹配到的字符串位置
 *********************************************************************************************************************/
os_size_t os_str_find(const char *str, const char *pattern);
/*********************************************************************************************************************
* 比较字符串
* str1：字符串1
* str2：字符串2
* return：-1：str1 < str2; 0: str1 == str2; 1: str1 > str2
*********************************************************************************************************************/
int8 os_str_cmp(const char *str1, const char *str2);
/*********************************************************************************************************************
* 连接字符串
* dest：字符串1，存放连接后的字符串
* src：字符串2
*********************************************************************************************************************/
char *os_str_cat(char *dest, const char *src);
#endif
