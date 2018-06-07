#ifndef __OS_VFS_H__
#define __OS_VFS_H__
#include "os_cpu_def.h"
#include "base/os_map.h"
#include "sched//os_sem.h"
#define MAX_FILE_NAME_SIZE 64
#define OS_FS_READ 1
#define OS_FS_WRITE 2
#define OS_FS_APPEND 4
#define OS_FS_CREATE 8

#define OS_DIR void *
#define OS_FILE void *

enum OS_SEEK_TYPE
{
	OS_FS_SEEK_SET,
	OS_FS_SEEK_CUR,
	OS_FS_SEEK_END
};

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
	os_sem_t *sem;
} os_file_info;

typedef struct os_dir
{
	os_map_iterator *itr;
	OS_DIR real_dir;
	os_file_info *vfile;
} os_dir;

typedef struct os_file
{
	OS_FILE real_file;
	os_file_info *vfile;
} os_file;

typedef struct os_file_operators
{
	void(*module_init)(os_file_info *mount_file);
	void(*module_uninit)(os_file_info *mount_file);
	void *(*ioctl)(os_file_info *mount_file, uint32 command, void *arg);
	OS_FILE(*open_file)(os_file_info *mount_file, const char *path, uint32 flags);
	void(*close_file)(os_file_info *mount_file, OS_FILE file);
	uint32(*read_file)(os_file_info *mount_file, OS_FILE file, void *data, uint32 len);
	uint32(*write_file)(os_file_info *mount_file, OS_FILE file, void *data, uint32 len);
	uint32(*seek_file)(os_file_info *mount_file, OS_FILE file, int64 offset, uint32 fromwhere);
	uint64(*tell_file)(os_file_info *mount_file, OS_FILE file);
} os_file_operators;

typedef struct os_fs_operators
{
	void (*mount)(os_file_info *mount_file, uint32 formatting);
	void (*unmount)(os_file_info *mount_file);
	void (*formatting)(os_file_info *mount_file);
	uint64(*total_size)(os_file_info *mount_file);
	uint64(*free_size)(os_file_info *mount_file);
	OS_DIR (*open_dir)(os_file_info *mount_file, const char *path);
	void (*close_dir)(os_file_info *mount_file, OS_DIR dir);
	uint32 (*read_dir)(os_file_info *mount_file, os_file_info *finfo, OS_DIR dir);
	uint32(*create_dir)(os_file_info *mount_file, const char *path);
	uint32(*create_file)(os_file_info *mount_file, const char *path);
	uint32 (*delete_dir)(os_file_info *mount_file, const char *path);
	uint32 (*delete_file)(os_file_info *mount_file, const char *path);
	uint32 (*move_file)(os_file_info *mount_file, const char *dest, const char *src);
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
VTOS_API uint32 os_mount(const char *mount_name, const char *dev_name, const char *fs_name, uint32 formatting);
/*********************************************************************************************************************
* 卸载文件系统
* mount_name：挂载的文件名
* return：0：卸载成功
*********************************************************************************************************************/
VTOS_API uint32 os_unmount(const char *mount_name);
/*********************************************************************************************************************
* 格式化文件系统
* mount_name：挂载的文件名
*********************************************************************************************************************/
VTOS_API void os_formatting(const char *mount_name);
/*********************************************************************************************************************
* 创建文件夹
* path：文件路径
* return：0：创建成功；1：创建失败
*********************************************************************************************************************/
VTOS_API uint32 os_create_dir(const char *path);
/*********************************************************************************************************************
* 创建文件
* path：文件路径
* return：0：创建成功；1：创建失败
*********************************************************************************************************************/
VTOS_API uint32 os_create_file(const char *path);
/*********************************************************************************************************************
* 打开一个目录
* path：文件路径
* return：目录
*********************************************************************************************************************/
VTOS_API os_dir *os_open_dir(const char *path);
/*********************************************************************************************************************
* 关闭一个目录
* dir：目录
*********************************************************************************************************************/
VTOS_API void os_close_dir(os_dir *dir);
/*********************************************************************************************************************
* 读取一个目录
* finfo：存放找到的info
* dir：目录
* return：0:读取成功
*********************************************************************************************************************/
VTOS_API uint32 os_read_dir(os_file_info *finfo, os_dir *dir);
/*********************************************************************************************************************
* 删除一个空目录
* path：路径
* return：0：成功；
*********************************************************************************************************************/
VTOS_API uint32 os_delete_dir(const char *path);
/*********************************************************************************************************************
* 删除一个文件
* path：路径
* return：0：成功；
*********************************************************************************************************************/
VTOS_API uint32 os_delete_file(const char *path);
/*********************************************************************************************************************
* 移动一个文件或者目录
* src：源目录
* dest：目标目录
* return：0：成功；
*********************************************************************************************************************/
VTOS_API uint32 os_move_file(const char *dest, const char *src);
/*********************************************************************************************************************
* 打开一个文件
* path：路径
* flags：可以选择FS_READ、FS_WRITE、FS_APPEND、FS_CREATE中的一个或组合
* return：文件对象
*********************************************************************************************************************/
VTOS_API os_file *os_open_file(const char *path, uint32 flags);
/*********************************************************************************************************************
* 关闭一个文件
* file：文件对象
*********************************************************************************************************************/
VTOS_API void os_close_file(os_file *file);
/*********************************************************************************************************************
* 读取文件
* file：文件对象
* data：数据
* len：读取的数据长度
* return：成功读取的大小
*********************************************************************************************************************/
VTOS_API uint32 os_read_file(os_file *file, void *data, uint32 len);
/*********************************************************************************************************************
* 写入文件
* file：文件对象
* data：数据
* len：写入的数据长度
* return：成功写入的大小
*********************************************************************************************************************/
VTOS_API uint32 os_write_file(os_file *file, void *data, uint32 len);
/*********************************************************************************************************************
* 移动文件指针
* file：文件对象
* offset：指针偏移
* fromwhere：FS_SEEK_SET，FS_SEEK_CUR，FS_SEEK_END
* return：0：成功；
*********************************************************************************************************************/
VTOS_API uint32 os_seek_file(os_file *file, int64 offset, uint32 fromwhere);
/*********************************************************************************************************************
* 获取指针位置
* file：文件对象
* return：指针位置
*********************************************************************************************************************/
VTOS_API uint64 os_tell_file(os_file *file);
/*********************************************************************************************************************
* io控制
* file：文件对象
* command：命令
* arg：参数
*********************************************************************************************************************/
VTOS_API void *os_ioctl(os_file *file, uint32 command, void *arg);
/*********************************************************************************************************************
* 获取总空间大小
* mount_name：挂载的文件名
*********************************************************************************************************************/
VTOS_API uint64 os_total_size(const char *mount_name);
/*********************************************************************************************************************
* 获取剩余内存大小
* mount_name：挂载的文件名
*********************************************************************************************************************/
VTOS_API uint64 os_free_size(const char *mount_name);
#endif // !__OS_VFS_H__
