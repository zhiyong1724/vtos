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

static void formatting(os_file_info *mount_file)
{
	fs_formatting((os_fs *)mount_file->arg);
}

static uint64 total_size(os_file_info *mount_file)
{
	os_fs *fs = (os_fs *)mount_file->arg;
	return get_all_cluster_num(&fs->cluster) * FS_CLUSTER_SIZE;
}

static uint64 free_size(os_file_info *mount_file)
{
	os_fs *fs = (os_fs *)mount_file->arg;
	return get_free_cluster_num(&fs->cluster) * FS_CLUSTER_SIZE;
}

static OS_DIR open_dir(os_file_info *mount_file, const char *path)
{
	return fs_open_dir(path, (os_fs *)mount_file->arg);
}

static void close_dir(os_file_info *mount_file, OS_DIR dir)
{
	fs_close_dir((dir_obj *)dir);
}

static uint32 read_dir(os_file_info *mount_file, os_file_info *finfo, OS_DIR dir)
{
	file_info info;
	uint32 ret = fs_read_dir(&info, dir, (os_fs *)mount_file->arg);
	os_str_cpy(finfo->name, info.name, MAX_FILE_NAME_SIZE);
	finfo->size = info.size;
	finfo->create_time = info.create_time;
	finfo->modif_time = info.modif_time;
	finfo->creator = info.creator;
	finfo->modifier = info.modifier;
	finfo->property = info.property;
	return ret;
}

static uint32 create_dir(os_file_info *mount_file, const char *path)
{
	return fs_create_dir(path, (os_fs *)mount_file->arg);
}

static uint32 create_file(os_file_info *mount_file, const char *path)
{
	return fs_create_file(path, (os_fs *)mount_file->arg);
}

static uint32 delete_dir(os_file_info *mount_file, const char *path)
{
	return fs_delete_dir(path, (os_fs *)mount_file->arg);
}

static uint32 delete_file(os_file_info *mount_file, const char *path)
{
	return fs_delete_file(path, (os_fs *)mount_file->arg);
}

static const os_fs_operators _emfs_operators =
{
.mount = mount,
.unmount = unmount,
.formatting = formatting,
.total_size = total_size,
.free_size = free_size,
.open_dir = open_dir,
.close_dir = close_dir,
.read_dir = read_dir,
.create_dir = create_dir,
.create_file = create_file,
.delete_dir = delete_dir,
.delete_file = delete_file,
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
