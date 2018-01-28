#include "fs/os_fs.h"
#include "fs/os_cluster.h"
#include <stdio.h>
void test()
{
	uint32 i = 48;
	uint32 j = 0;
	/*cluster_manager_init();
	for (i = 48 ; i != 0; i++)
	{
		j = cluster_alloc();
		if (j != i)
		{
			break;
		}
	}
	flush2();*/
	cluster_manager_load(16, 20);
	cluster_alloc();
	/*for (i = 48 ; i < 65536; i++)
	{
		cluster_free(i);
	}*/

	/*for (i = 65535 ; i >= 48; i--)
	{
		cluster_free(i);
	}*/

	/*for (i = 48 ; i != 0; i++)
	{
		j = cluster_alloc();
		if (j != i)
		{
			break;
		}
	}*/
}



