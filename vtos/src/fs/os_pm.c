#include "fs/os_pm.h"
static int i = 1;
cluster_info cluster_alloc(int num)
{
	cluster_info cluster;
	cluster.id = i;
	cluster.num = num;
	i++;
	return cluster;
}

void cluster_free(uint32 page_id)
{
}