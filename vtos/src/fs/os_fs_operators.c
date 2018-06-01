#include "os_fs.h"
#include "vfs/os_vfs.h"
#include "base/os_string.h"
static void mount(os_file_info *mount_file, uint32 formatting)
{
	uint32 dev_id = os_atou(&mount_file->dev->name[5]);
	mount_file->arg = fs_mount(dev_id, formatting);
}

static void unmount(os_file_info *mount_file)
{
	fs_unmount((os_fs *)mount_file->arg);
}

static const os_fs_operators _emfs_operators = 
{
.mount = mount,
.unmount = unmount,
.create_dir = NULL,
.create_file = NULL,
.open_dir = NULL,
.close_dir = NULL,
.read_dir = NULL,
.delete_dir = NULL,
.delete_file = NULL,
.move_file = NULL,
.copy_file = NULL
};

static const os_file_operators _emfs_file_operators =
{
.module_init = NULL,
.module_uninit = NULL,
.os_ioctl = NULL,
.open_file = NULL,
.close_file = NULL,
.read_file = NULL,
.write_file = NULL,
.seek_file = NULL,
.tell_file = NULL
};

static const os_file_system _emfs =
{
.name = "emfs",
.file_operators = &_emfs_file_operators,
.fs_operators = &_emfs_operators
};
