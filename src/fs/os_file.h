#ifndef __OS_FILE_H__
#define __OS_FILE_H__
#include "fs/os_cluster.h"
#define FS_MAX_INDEX_NUM (FS_CLUSTER_SIZE / sizeof(uint32))
/*********************************************************************************************************************
* 写小文件，内容大小在一簇以内
* id：要写入的簇id
* index：写入位置
* data：写入数据
* size：数据大小
* cluster：os_cluster对象
* return：成功写入大小
*********************************************************************************************************************/
uint32 little_file_data_write(uint32 id, uint64 index, void *data, uint32 size, os_cluster *cluster);
/*********************************************************************************************************************
* 读取小文件，内容大小在一簇以内
* id：要读取的簇id
* index：读取位置
* data：读取数据
* size：数据大小
* cluster：os_cluster对象
* return：成功读取大小
*********************************************************************************************************************/
uint32 little_file_data_read(uint32 id, uint64 index, void *data, uint32 size, os_cluster *cluster);
/*********************************************************************************************************************
* 写入文件，内容大小大于一簇
* id：要写入的首簇id
* count：簇计数
* index：写入位置
* data：写入数据
* size：数据大小
* cluster：os_cluster对象
* return：tree id
*********************************************************************************************************************/
uint32 file_data_write(uint32 id, uint32 *count, uint64 index, uint8 *data, uint32 size, os_cluster *cluster);
/*********************************************************************************************************************
* 读取文件，内容大小大于一簇
* id：要读取的首簇id
* count：簇计数
* index：读取位置
* data：读取数据
* size：数据大小
* cluster：os_cluster对象
* return：tree id
*********************************************************************************************************************/
uint32 file_data_read(uint32 id, uint32 count, uint64 index, uint8 *data, uint32 size, os_cluster *cluster);
/*********************************************************************************************************************
* 移除文件数据
* id：数据簇id
* count：占用簇数量
* cluster：os_cluster对象
*********************************************************************************************************************/
void file_data_remove(uint32 id, uint32 count, os_cluster *cluster);
#endif