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
/*********************************************************************************************************************
* 格式化文件系统
* return：0：格式化成功；
*********************************************************************************************************************/
uint32 fs_formatting();
/*********************************************************************************************************************
* 加载文件系统
* return：0：加载成功；
*********************************************************************************************************************/
uint32 fs_loading();
/*********************************************************************************************************************
* 释放文件系统
*********************************************************************************************************************/
void fs_unloading();
uint32 create_dir(const char *path);
uint32 create_file(const char *path);
uint32 delete_dir();
uint32 delete_file();
uint32 open_file();
uint32 close_file();
uint32 read_file();
uint32 write_file();
uint32 seek_file();
dir_ctl *open_dir();
void close_dir(dir_ctl *dir);
file_info *read_dir(dir_ctl *dir);
uint32 find_file();
void test();
#endif