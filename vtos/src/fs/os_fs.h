#ifndef __OS_FS_H__
#define __OS_FS_H__
#include "fs/os_fs_def.h"
#include "fs/os_dentry.h"
#include "fs/os_cluster.h"
#include "fs/os_journal.h"
#include "base/os_tree.h"
#define FS_READ 1
#define FS_WRITE 2
#define FS_APPEND 4
#define FS_CREATE 8
enum SEEK_TYPE
{
	FS_SEEK_SET, 
	FS_SEEK_CUR,
	FS_SEEK_END
};
typedef struct dir_obj
{
	uint32 id;
	fnode *parent;
	fnode *cur; 
	uint32 index; 
} dir_obj;

typedef struct finfo_node
{
	tree_node_type_def tree_node_structrue;
	uint32 count;
	uint32 cluster_id;
	uint32 index;
	uint32 flag;
	file_info finfo;
} finfo_node;

typedef struct file_obj
{
	uint32 flags;
	uint64 index;
	finfo_node *node;
} file_obj;

typedef struct os_fs
{
	uint32 is_update_super;
	super_cluster *super;
	fnode *root;
	finfo_node *open_file_tree;
	os_cluster cluster;
	os_dentry dentry;
	os_journal journal;
} os_fs;
/*********************************************************************************************************************
* 挂载文件系统
* dev_id：设备id
* formatting：1：挂载并格式化磁盘
* return：文件系统对象
*********************************************************************************************************************/
os_fs *fs_mount(uint32 dev_id, uint32 formatting);
/*********************************************************************************************************************
* 卸载文件系统
* fs：文件系统对象
*********************************************************************************************************************/
void fs_unmount(os_fs *fs);
/*********************************************************************************************************************
* 格式化文件系统
* fs：os_fs对象
*********************************************************************************************************************/
void fs_formatting(os_fs *fs);
/*********************************************************************************************************************
* 创建文件夹
* path：文件路径
* fs：os_fs对象
* return：0：创建成功；1：创建失败
*********************************************************************************************************************/
uint32 create_dir(const char *path, os_fs *fs);
/*********************************************************************************************************************
* 创建文件
* path：文件路径
* fs：os_fs对象
* return：0：创建成功；1：创建失败
*********************************************************************************************************************/
uint32 create_file(const char *path, os_fs *fs);
/*********************************************************************************************************************
* 打开一个目录
* path：文件路径
* fs：os_fs对象
* return：目录
*********************************************************************************************************************/
dir_obj *open_dir(const char *path, os_fs *fs);
/*********************************************************************************************************************
* 关闭一个目录
* dir：目录
*********************************************************************************************************************/
void close_dir(dir_obj *dir);
/*********************************************************************************************************************
* 读取一个目录
* finfo：存放找到的info
* dir：目录
* fs：os_fs对象
* return：0:读取成功
*********************************************************************************************************************/
uint32 read_dir(file_info *finfo, dir_obj *dir, os_fs *fs);
/*********************************************************************************************************************
* 删除一个空目录
* path：路径
* fs：os_fs对象
* return：0：成功；
*********************************************************************************************************************/
uint32 delete_dir(const char *path, os_fs *fs);
/*********************************************************************************************************************
* 删除一个文件
* path：路径
* fs：os_fs对象
* return：0：成功；
*********************************************************************************************************************/
uint32 delete_file(const char *path, os_fs *fs);
/*********************************************************************************************************************
* 移动一个文件或者目录
* src：源目录
* dest：目标目录
* fs：os_fs对象
* return：0：成功；
*********************************************************************************************************************/
uint32 move_file(const char *dest, const char *src, os_fs *fs);
/*********************************************************************************************************************
* 打开一个文件
* path：路径
* flags：可以选择FS_READ、FS_WRITE、FS_APPEND、FS_CREATE中的一个或组合
* fs：os_fs对象
* return：文件对象
*********************************************************************************************************************/
file_obj *open_file(const char *path, uint32 flags, os_fs *fs);
/*********************************************************************************************************************
* 关闭一个文件
* file：文件对象
* fs：os_fs对象
*********************************************************************************************************************/
void close_file(file_obj *file, os_fs *fs);
/*********************************************************************************************************************
* 读取文件
* file：文件对象
* data：数据
* len：读取的数据长度
* fs：os_fs对象
* return：成功读取的大小
*********************************************************************************************************************/
uint32 read_file(file_obj *file, void *data, uint32 len, os_fs *fs);
/*********************************************************************************************************************
* 写入文件
* file：文件对象
* data：数据
* len：写入的数据长度
* fs：os_fs对象
* return：成功写入的大小
*********************************************************************************************************************/
uint32 write_file(file_obj *file, void *data, uint32 len, os_fs *fs);
/*********************************************************************************************************************
* 移动文件指针
* file：文件对象
* offset：指针偏移
* fromwhere：FS_SEEK_SET，FS_SEEK_CUR，FS_SEEK_END
* return：0：成功；
*********************************************************************************************************************/
uint32 seek_file(file_obj *file, int64 offset, uint32 fromwhere);
/*********************************************************************************************************************
* 获取指针位置
* file：文件对象
* return：指针位置
*********************************************************************************************************************/
uint64 tell_file(file_obj *file);
/*********************************************************************************************************************
* 开启日志功能
* fs：os_fs对象
*********************************************************************************************************************/
void open_journal(os_fs *fs);
/*********************************************************************************************************************
* 关闭日志功能
* fs：os_fs对象
*********************************************************************************************************************/
void close_journal(os_fs *fs);
#endif