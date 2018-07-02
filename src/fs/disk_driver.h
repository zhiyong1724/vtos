﻿#ifndef __DISK_DRIVER_H__
#define __DISK_DRIVER_H__
#include "vfs/os_vfs.h"
/*********************************************************************************************************************
* 注册磁盘设备
*********************************************************************************************************************/
uint32 register_disk_dev();
/*********************************************************************************************************************
* 卸载磁盘设备
*********************************************************************************************************************/
uint32 unregister_disk_dev();
#endif