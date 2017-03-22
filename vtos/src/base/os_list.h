#ifndef _OS_LIST_H
#define _OS_LIST_H
#include "os_cpu.h"
typedef struct list_node_type_def
{
	struct list_node_type_def *pre_node;
	struct list_node_type_def *next_node;
} list_node_type_def;

void os_insert_to_front(list_node_type_def **handle, list_node_type_def *node);

void os_insert_to_back(list_node_type_def **handle, list_node_type_def *node);

void os_remove_from_list(list_node_type_def **handle, list_node_type_def *node);

list_node_type_def *os_get_back_from_list(list_node_type_def **handle);
#endif