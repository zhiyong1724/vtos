#ifndef __OS_MULTISET_H__
#define __OS_MULTISET_H__
#include "os_set.h"
#pragma pack()
typedef struct os_multiset_iterator
{
	os_set_iterator set_iterator;
} os_multiset_iterator;
#pragma pack()

typedef struct os_multiset
{
	os_set set;
} os_multiset;

/*********************************************************************************************************************
* 初始化容器
* obj 容器对象
* unit_size 元素大小
*********************************************************************************************************************/
void os_multiset_init(os_multiset *obj, os_size_t unit_size);
/*********************************************************************************************************************
* 释放容器
* obj 容器对象
*********************************************************************************************************************/
void os_multiset_free(os_multiset *obj);
/*********************************************************************************************************************
* 获取容器大小
* obj 容器对象
*********************************************************************************************************************/
os_size_t os_multiset_size(os_multiset *obj);
/*********************************************************************************************************************
* 判断容器是否为空
* obj 容器对象
*********************************************************************************************************************/
os_size_t os_multiset_empty(os_multiset *obj);
/*********************************************************************************************************************
* 插入元素
* obj 容器对象
* data 数据
* return 0 成功插入
*********************************************************************************************************************/
os_size_t os_multiset_insert(os_multiset *obj, void *data);
/*********************************************************************************************************************
* 查找元素
* obj 容器对象
* data 数据
* return 对应的迭代器
*********************************************************************************************************************/
os_multiset_iterator *os_multiset_find(os_multiset *obj, void *data);
/*********************************************************************************************************************
* 移除元素
* obj 容器对象
* itr 要移除的数据
* return 0:成功移除
*********************************************************************************************************************/
os_size_t os_multiset_erase(os_multiset *obj, os_multiset_iterator *itr);
/*********************************************************************************************************************
* 清空所有元素
* obj 容器对象
*********************************************************************************************************************/
void os_multiset_clear(os_multiset *obj);
/*********************************************************************************************************************
* 解引用迭代器
* itr 迭代器
* retuan 数据指针
*********************************************************************************************************************/
void *os_multiset_value(os_multiset_iterator *itr);
/*********************************************************************************************************************
* 返回第一个值的迭代器
* obj 容器对象
* retuan 迭代器
*********************************************************************************************************************/
os_multiset_iterator *os_multiset_begin(os_multiset *obj);
/*********************************************************************************************************************
* 返回下一个迭代器
* obj 容器对象
* itr 迭代器
* retuan 下一个迭代器
*********************************************************************************************************************/
os_multiset_iterator *os_multiset_next(os_multiset *obj, os_multiset_iterator *itr);
#endif