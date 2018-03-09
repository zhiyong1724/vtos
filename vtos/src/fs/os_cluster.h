#ifndef __OS_CLUSTER_H__
#define __OS_CLUSTER_H__
#include "fs/os_fs_def.h"
#define FIRST_CLUSTER_MANAGER_ID (SUPER_CLUSTER_ID + 3)
#pragma pack(1)
struct cluster_manager
{
	uint32 used_cluster_count;
	uint32 cur_index;
};
#pragma pack()

struct cluster_controler
{
	disk_info dinfo;
	struct cluster_manager *pcluster_manager;
	uint32 cache_id;
	uint8 *bitmap;
	uint32 divisor;
	uint32 bitmap_size;
	uint32 total_cluster_count;
};
/*********************************************************************************************************************
* 把缓存数据写入磁盘
*********************************************************************************************************************/
void flush();
/*********************************************************************************************************************
* 转换字节序
* src：源数据
* return：转换后的数据
*********************************************************************************************************************/
uint32 convert_endian(uint32 src);
/*********************************************************************************************************************
* 转换字节序
* src：源数据
* return：转换后的数据
*********************************************************************************************************************/
uint64 convert_endian64(uint64 src);
/*********************************************************************************************************************
* 初始化簇管理器
*********************************************************************************************************************/
void cluster_manager_init();
/*********************************************************************************************************************
* 初始化簇控制器
*********************************************************************************************************************/
void cluster_controler_init();
/*********************************************************************************************************************
* 释放簇管理器
*********************************************************************************************************************/
void uninit();
/*********************************************************************************************************************
* 簇分配函数，和cluster_free联合使用
* return：分配到的簇id
*********************************************************************************************************************/
uint32 cluster_alloc();
/*********************************************************************************************************************
* 簇释放函数，和cluster_alloc联合使用
* cluster_id：要释放的簇id
*********************************************************************************************************************/
void cluster_free(uint32 cluster_id);
/*********************************************************************************************************************
* 簇读取函数
* cluster_id：簇id
* data：读取存放的数据
*********************************************************************************************************************/
void cluster_read(uint32 cluster_id, uint8 *data);
/*********************************************************************************************************************
* 簇写入函数
* cluster_id：簇id
* data：写入的数据
*********************************************************************************************************************/
void cluster_write(uint32 cluster_id, uint8 *data);
/*********************************************************************************************************************
* 加载簇管理器
*********************************************************************************************************************/
void cluster_manager_load();
/*********************************************************************************************************************
* 获取总簇数量
*********************************************************************************************************************/
uint32 get_all_cluster_num();
/*********************************************************************************************************************
* 获取剩余簇数量
*********************************************************************************************************************/
uint32 get_free_cluster_num();
#endif