#ifndef __OS_FS_H__
#define __OS_FS_H__
#include "fs/os_fs_def.h"
#include "fs/os_dentry.h"
typedef struct dir_ctl
{
	uint32 id;
	fnode *parent;
	fnode *cur; 
	uint32 index; 
} dir_ctl;
void fs_unloading();
void test();
#endif