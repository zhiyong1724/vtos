#ifndef __OS_DENTRY_H__
#define __OS_DENTRY_H__
#include "fs/os_cluster.h"
#include "fs/os_fs_def.h"
/*********************************************************************************************************************
* 插入文件信息
* root：目录项根节点
* finfo：要插入的文件信息
* return：目录项根节点
*********************************************************************************************************************/
fnode *insert_to_btree(fnode *root, file_info *finfo);
/*********************************************************************************************************************
* 删除文件信息
* root：目录项根节点
* finfo：要删除的文件信息
*********************************************************************************************************************/
fnode *remove_from_btree(fnode *root, file_info *finfo);
#endif