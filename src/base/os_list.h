#ifndef __OS_LIST_H__
#define __OS_LIST_H__
#include "os_cpu_def.h"
typedef struct list_node_type_def
{
	struct list_node_type_def *pre_node;
	struct list_node_type_def *next_node;
} list_node_type_def;

typedef struct os_list
{
	list_node_type_def *list;
	os_size_t unit_size;
	os_size_t size;
} os_list;
/*********************************************************************************************************************
* 从前面插入节点
* handle 链表头
* node 新节点
*********************************************************************************************************************/
void os_insert_to_front(list_node_type_def **handle, list_node_type_def *node);
/*********************************************************************************************************************
* 从后面插入节点
* handle 链表头
* node 新节点
*********************************************************************************************************************/
void os_insert_to_back(list_node_type_def **handle, list_node_type_def *node);
/*********************************************************************************************************************
* 删除一个节点
* handle 链表头
* node 要删除的节点
*********************************************************************************************************************/
void os_remove_from_list(list_node_type_def **handle, list_node_type_def *node);
/*********************************************************************************************************************
* 获取一个最后面的节点
* handle 链表头
* return 获取到的节点
*********************************************************************************************************************/
list_node_type_def *os_get_back_from_list(list_node_type_def **handle);
/*********************************************************************************************************************
* 初始化容器
* obj 容器对象
* unit_size 元素大小
*********************************************************************************************************************/
void os_list_init(os_list *obj, os_size_t unit_size);
/*********************************************************************************************************************
* 释放容器
* obj 容器对象
*********************************************************************************************************************/
void os_list_free(os_list *obj);
/*********************************************************************************************************************
* 获取容器大小
* obj 容器对象
*********************************************************************************************************************/
os_size_t os_list_size(os_list *obj);
/*********************************************************************************************************************
* 判断容器是否为空
* obj 容器对象
*********************************************************************************************************************/
os_size_t os_list_empty(os_list *obj);
/*********************************************************************************************************************
* 从后面添加元素
* obj 容器对象
* data 数据
* return 元素数目
*********************************************************************************************************************/
os_size_t os_list_push_back(os_list *obj, void *data);
/*********************************************************************************************************************
* 从前面添加元素
* obj 容器对象
* data 数据
* return 元素数目
*********************************************************************************************************************/
os_size_t os_list_push_front(os_list *obj, void *data);
/*********************************************************************************************************************
* 插入元素
* obj 容器对象
* data 数据
* n 插入位置
* return 元素数目
*********************************************************************************************************************/
os_size_t os_list_insert(os_list *obj, void *data, os_size_t n);
/*********************************************************************************************************************
* 访问元素
* obj 容器对象
* return 返回的数据指针
*********************************************************************************************************************/
void *os_list_back(os_list *obj);
/*********************************************************************************************************************
* 访问元素
* obj 容器对象
* return 返回的数据指针
*********************************************************************************************************************/
void *os_list_front(os_list *obj);
/*********************************************************************************************************************
* 访问元素
* obj 容器对象
* n 元素位置
* return 返回的数据指针
*********************************************************************************************************************/
void *os_list_at(os_list *obj, os_size_t n);
/*********************************************************************************************************************
* 移除元素
* obj 容器对象
* n 元素位置
* return 0:成功移除
*********************************************************************************************************************/
os_size_t os_list_erase(os_list *obj, os_size_t n);
/*********************************************************************************************************************
* 清空所有元素
* obj 容器对象
*********************************************************************************************************************/
void os_list_clear(os_list *obj);
/*********************************************************************************************************************
* 从后面移除一个元素
* obj 容器对象
* return 0:成功移除
*********************************************************************************************************************/
os_size_t os_list_pop_back(os_list *obj);
/*********************************************************************************************************************
* 从前面移除一个元素
* obj 容器对象
* return 0:成功移除
*********************************************************************************************************************/
os_size_t os_list_pop_front(os_list *obj);
#endif