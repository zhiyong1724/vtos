#ifndef __OS_VFS_H__
#define __OS_VFS_H__
#include "os_cpu_def.h"
#include "base/os_map.h"
#define MAX_FILE_NAME_SIZE 64
#define OS_FS_READ 1
#define OS_FS_WRITE 2
#define OS_FS_APPEND 4
#define OS_FS_CREATE 8

#define OS_DIR void *
#define OS_FILE void *

enum DISK_OPERATE
{
	DISK_INFO,
	DISK_READ,
	DISK_WRITE,
};

struct os_file_operators;
struct os_fs_operators;

typedef struct os_disk_info
{
	uint32 first_page_id;
	uint32 page_size;
	uint32 page_count;
} os_disk_info;

typedef struct os_file_info
{
	char name[MAX_FILE_NAME_SIZE];
	uint64 size;
	uint64 create_time;
	uint64 modif_time;
	uint32 creator;
	uint32 modifier;
	uint32 property;  //.10 dir or file;.9 only sys w;.876 user r,w,x;.543 group r,w,x;.210 other r,w,x
	os_map *files;
	struct os_file_info *dev;
	void *arg;
	const struct os_file_operators *file_operators;
	const struct os_fs_operators *fs_operators;
} os_file_info;

typedef struct os_dir
{
	os_map_iterator *itr;
	OS_DIR real_dir;
	os_file_info *vfile;
} os_dir;

typedef struct os_file_operators
{
	void(*module_init)();
	void(*module_uninit)();
	void *(*os_ioctl)(uint32 command, void *arg);
	OS_FILE(*open_file)(const char *path, uint32 flags);
	void(*close_file)(OS_FILE file);
	uint32(*read_file)(OS_FILE file, void *data, uint32 len);
	uint32(*write_file)(OS_FILE file, void *data, uint32 len);
	uint32(*seek_file)(OS_FILE file, int64 offset, uint32 fromwhere);
	uint64(*tell_file)(OS_FILE file);
} os_file_operators;

typedef struct os_fs_operators
{
	void (*mount)(os_file_info *mount_file, uint32 formatting);
	void (*unmount)(os_file_info *mount_file);
	uint32 (*create_dir)(const char *path);
	uint32 (*create_file)(const char *path);
	OS_DIR (*open_dir)(const char *path);
	void (*close_dir)(OS_DIR dir);
	uint32 (*read_dir)(os_file_info *finfo, OS_DIR dir);
	uint32 (*delete_dir)(const char *path);
	uint32 (*delete_file)(const char *path);
	uint32 (*move_file)(const char *dest, const char *src);
	uint32 (*copy_file)(const char *dest, const char *src);
} os_fs_operators;

typedef struct os_file_system
{
	const char *name;
	const os_file_operators *file_operators;
	const os_fs_operators *fs_operators;
} os_file_system;

struct os_vfs
{
	os_file_info root;
};

/*********************************************************************************************************************
* 初始化vfs
*********************************************************************************************************************/
void os_vfs_init();
/*********************************************************************************************************************
* 卸载vfs
*********************************************************************************************************************/
void os_vfs_uninit();
/*********************************************************************************************************************
* 创建虚拟文件
* path：文件路径
* return：
*********************************************************************************************************************/
os_file_info *os_create_vfile(const char *path, const os_file_operators *file_operators, const os_fs_operators *fs_operators);
/*********************************************************************************************************************
* 删除一个虚拟文件
* path：路径
* return：0：成功；
*********************************************************************************************************************/
uint32 os_delete_vfile(const char *path);
/*********************************************************************************************************************
* 注册一个设备文件，必须在os_vfs_init后使用
* name：设备名
* file_operators：设备操作接口
* return：0：成功；
*********************************************************************************************************************/
uint32 os_register_dev(const char *name, const os_file_operators *file_operators);
/*********************************************************************************************************************
* 移除注册一个设备文件
* name：设备名
* return：0：成功；
*********************************************************************************************************************/
uint32 os_unregister_dev(const char *name);
/*********************************************************************************************************************
* 挂载文件系统
* mount_name：挂载的文件名
* dev_name：设备名
* fs_name：要挂载的文件系统名字
* formatting：是否需要格式化？1：进行格式化，0：不进行格式化
* return：0：挂载成功
*********************************************************************************************************************/
uint32 os_mount(const char *mount_name, const char *dev_name, const char *fs_name, uint32 formatting);
/*********************************************************************************************************************
* 卸载文件系统
* mount_name：挂载的文件名
* return：0：卸载成功
*********************************************************************************************************************/
uint32 os_unmount(const char *mount_name);
/*********************************************************************************************************************
* 创建文件
* path：文件路径
* return：0：创建成功；1：创建失败
*********************************************************************************************************************/
uint32 os_create_file(const char *path);
/*********************************************************************************************************************
* 创建文件夹
* path：文件路径
* return：0：创建成功；1：创建失败
*********************************************************************************************************************/
uint32 os_create_dir(const char *path);
/*********************************************************************************************************************
* 创建文件
* path：文件路径
* return：0：创建成功；1：创建失败
*********************************************************************************************************************/
uint32 os_create_file(const char *path);
/*********************************************************************************************************************
* 打开一个目录
* path：文件路径
* return：目录
*********************************************************************************************************************/
os_dir *os_open_dir(const char *path);
/*********************************************************************************************************************
* 关闭一个目录
* dir：目录
*********************************************************************************************************************/
void os_close_dir(os_dir *dir);
/*********************************************************************************************************************
* 读取一个目录
* finfo：存放找到的info
* dir：目录
* return：0:读取成功
*********************************************************************************************************************/
uint32 os_read_dir(os_file_info *finfo, os_dir *dir);
/*********************************************************************************************************************
* 删除一个空目录
* path：路径
* return：0：成功；
*********************************************************************************************************************/
uint32 os_delete_dir(const char *path);
/*********************************************************************************************************************
* 删除一个文件
* path：路径
* return：0：成功；
*********************************************************************************************************************/
uint32 os_delete_file(const char *path);
/*********************************************************************************************************************
* 移动一个文件或者目录
* src：源目录
* dest：目标目录
* return：0：成功；
*********************************************************************************************************************/
uint32 os_move_file(const char *dest, const char *src);
/*********************************************************************************************************************
* 复制一个文件或者目录
* src：源目录
* dest：目标目录
* return：0：成功；
*********************************************************************************************************************/
uint32 os_copy_file(const char *dest, const char *src);
/*********************************************************************************************************************
* 打开一个文件
* path：路径
* flags：可以选择FS_READ、FS_WRITE、FS_APPEND、FS_CREATE中的一个或组合
* return：文件对象
*********************************************************************************************************************/
OS_FILE os_open_file(const char *path, uint32 flags);
/*********************************************************************************************************************
* 关闭一个文件
* file：文件对象
*********************************************************************************************************************/
void os_close_file(OS_FILE file);
/*********************************************************************************************************************
* 读取文件
* file：文件对象
* data：数据
* len：读取的数据长度
* return：成功读取的大小
*********************************************************************************************************************/
uint32 os_read_file(OS_FILE file, void *data, uint32 len);
/*********************************************************************************************************************
* 写入文件
* file：文件对象
* data：数据
* len：写入的数据长度
* return：成功写入的大小
*********************************************************************************************************************/
uint32 os_write_file(OS_FILE file, void *data, uint32 len);
/*********************************************************************************************************************
* 移动文件指针
* file：文件对象
* offset：指针偏移
* fromwhere：FS_SEEK_SET，FS_SEEK_CUR，FS_SEEK_END
* return：0：成功；
*********************************************************************************************************************/
uint32 os_seek_file(OS_FILE file, int64 offset, uint32 fromwhere);
/*********************************************************************************************************************
* 获取指针位置
* file：文件对象
* return：指针位置
*********************************************************************************************************************/
uint64 os_tell_file(OS_FILE file);
/*********************************************************************************************************************
* io控制
* command：命令
* arg：参数
*********************************************************************************************************************/
void *os_ioctl(uint32 command, void *arg);
#endif // !__OS_VFS_H__
