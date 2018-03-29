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
	uint8 *buff;
	os_size_t unit_size;
	os_size_t size;
	os_size_t max_size;
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

#endif