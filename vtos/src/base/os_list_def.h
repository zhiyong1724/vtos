#ifndef _OS_LIST_DEF_H
#define _OS_LIST_DEF_H
#include "os_cpu.h"
typedef struct list_node_type_def
{
	struct list_node_type_def *pre_node;
	struct list_node_type_def *next_node;
} list_node_type_def;
#endif