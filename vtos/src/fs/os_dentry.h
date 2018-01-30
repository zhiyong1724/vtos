#ifndef __OS_DENTRY_H__
#define __OS_DENTRY_H__
#include "fs/os_cluster.h"
typedef struct tnode
{
	uint32 node_id;
	uint32 pointers[FS_MAX_KEY_NUM + 1];
	uint32 num;
	uint32 leaf;
	uint32 non[12];
} tnode;

typedef struct fnode
{
	tnode head;
	file_info finfo[FS_MAX_KEY_NUM];

} fnode;
/*********************************************************************************************************************
* 插入文件信息
* root：目录项根节点
* finfo：要插入的文件信息
* status：0:插入成功；1：插入失败
* return：目录项根节点
*********************************************************************************************************************/
fnode *insert_to_btree(fnode *root, file_info *finfo, uint32 *status);
/*********************************************************************************************************************
* 删除文件信息
* root：目录项根节点
* status：0:删除成功；1：删除失败
* finfo：要删除的文件信息
*********************************************************************************************************************/
fnode *remove_from_btree(fnode *root, file_info *finfo, uint32 *status);
#endif