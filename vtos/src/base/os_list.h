#ifndef __OS_LIST_H__
#define __OS_LIST_H__
#include "os_list_def.h"

void os_insert_to_front(list_node_type_def **handle, list_node_type_def *node);

void os_insert_to_back(list_node_type_def **handle, list_node_type_def *node);

void os_remove_from_list(list_node_type_def **handle, list_node_type_def *node);

list_node_type_def *os_get_back_from_list(list_node_type_def **handle);
#endif