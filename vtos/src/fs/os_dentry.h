#ifndef __OS_DENTRY_H__
#define __OS_DENTRY_H__
#include "fs/os_cluster.h"
#include "base/os_map.h"
#define FS_TNODE_SIZE 128
typedef void(*on_move_info)(uint32 id, uint32 index, uint32 key, uint32 dev_id);
#pragma pack(1)
typedef struct tnode
{
	uint32 node_id;
	uint32 pointers[FS_MAX_KEY_NUM + 1];
	uint32 num;
	uint32 leaf;
	uint32 next;
	uint32 non[11];
} tnode;
#pragma pack()

#pragma pack(1)
typedef struct fnode
{
	tnode head;
	file_info finfo[FS_MAX_KEY_NUM];

} fnode;
#pragma pack()

typedef struct os_dentry
{
	on_move_info move_callback;
	os_map fnodes;
	os_cluster *cluster;
} os_dentry;

typedef struct fnode_h
{
	fnode *node;
	uint32 count;
	uint32 flag;
} fnode_h;
/*********************************************************************************************************************
* 初始化
*********************************************************************************************************************/
void os_dentry_init(os_dentry *dentry, on_move_info call_back, os_cluster *cluster);
/*********************************************************************************************************************
* 释放资源
*********************************************************************************************************************/
void os_dentry_uninit(os_dentry *dentry);
/*********************************************************************************************************************
* 插入文件信息
* root：目录项根节点
* finfo：要插入的文件信息
* dentry：os_dentry对象
* return：目录项根节点
*********************************************************************************************************************/
fnode *insert_to_btree(fnode *root, file_info *finfo, os_dentry *dentry);
/*********************************************************************************************************************
* 删除文件信息
* root：目录项根节点
* name：文件名
* dentry：os_dentry对象
* return：目录项根节点
*********************************************************************************************************************/
fnode *remove_from_btree(fnode *root, const char *name, os_dentry *dentry);
/*********************************************************************************************************************
* 查找文件
* root：目录项根节点
* index：文件在fnode的指针
* name：要查找的文件名字
* dentry：os_dentry对象
* return：文件所在的fnode
*********************************************************************************************************************/
fnode *find_from_tree(fnode *root, uint32 *index, const char *name, os_dentry *dentry);
/*********************************************************************************************************************
* 判读文件是否存在
* root：目录项根节点
* name：要查找的文件名字
* dentry：os_dentry对象
* return：1：文件存在；0：文件不存在
*********************************************************************************************************************/
uint32 finfo_is_exist(fnode *root, const char *name, os_dentry *dentry);
/*********************************************************************************************************************
* 遍历文件
* finfo:存放查找到的值
* args:需要外部提供用来存储的变量，需要外部初始化为0和NULL
* dentry：os_dentry对象
* return:0：查找成功：1：查找失败
*********************************************************************************************************************/
uint32 query_finfo(file_info *finfo, uint32 *id, fnode **parent, fnode **cur, uint32 *index, os_dentry *dentry);
/*********************************************************************************************************************
* 把fnode写入磁盘
* node：要写入的fnode
* dentry：os_dentry对象
*********************************************************************************************************************/
void fnode_flush(fnode *node, os_dentry *dentry);
/*********************************************************************************************************************
* 缓存的fnodes写入磁盘
* root:根节点指针，根节点有缓存，不能释放
* dentry：os_dentry对象
*********************************************************************************************************************/
void fnodes_flush(fnode *root, os_dentry *dentry);
/*********************************************************************************************************************
* 加载fnode,如果已经缓存有则直接返回缓存
* id：cluster id
* dentry：os_dentry对象
* return：加载到的fnode；NULL：加载失败
*********************************************************************************************************************/
fnode *fnode_load(uint32 id, os_dentry *dentry);
/*********************************************************************************************************************
* 释放fnode,如果已经缓存则不释放
* node：要释放的节点
* dentry：os_dentry对象
* return：0:成功释放
*********************************************************************************************************************/
uint32 fnode_free(fnode *node, os_dentry *dentry);
/*********************************************************************************************************************
* 强制释放fnode
* node：要释放的节点
* dentry：os_dentry对象
* return：0:成功释放
*********************************************************************************************************************/
uint32 fnode_free_f(fnode *node, os_dentry *dentry);
/*********************************************************************************************************************
* 把fnose插入到缓存
* node：fnose
* dentry：os_dentry对象
*********************************************************************************************************************/
void insert_to_fnodes(fnode *node, os_dentry *dentry);
/*********************************************************************************************************************
* 设置fnode的属性到等待写入
* node：fnode
* dentry：os_dentry对象
*********************************************************************************************************************/
void add_flush(fnode *node, os_dentry *dentry);
#endif