#include "os_vfs.h"
#include "base/os_string.h"
#include "vtos.h"
#include "base/os_vector.h"
#include "fs/os_fs_operators.c"
static struct os_vfs _os_vfs;
static const os_file_system *_file_systems[] =
{
	&_emfs,
	NULL
};
static const char *DEV_PATH = "/dev";
static const char *MNT_PATH = "/mnt";
static char *pretreat_path(const char *path)
{
	char *buff = NULL;
	if ('/' == path[0])
	{
		int32 path_len = os_str_len(path);
		os_vector names;
		os_vector_init(&names, sizeof(int32));
		buff = (char *)os_malloc(path_len + 1);
		os_str_cpy(buff, path, path_len + 1);
		int32 i;
		for (i = 0; i < path_len;)
		{
			for (; '/' == path[i] && i < path_len; i++)
			{
				buff[i] = '\0';
			}
			if (i < path_len)
			{
				os_vector_push_back(&names, &i);
			}
			for (; path[i] != '/' && i < path_len; i++)
			{
			}
		}
		int32 size = (int32)os_vector_size(&names);
		os_vector filter_names;
		os_vector_init(&filter_names, sizeof(int32));
		for (i = 0; i < size; i++)
		{
			int32 *index = (int32 *)os_vector_at(&names, i);
			if (os_str_cmp(&buff[*index], ".") == 0)
			{
				continue;
			}
			else if (os_str_cmp(&buff[*index], "..") == 0)
			{
				if (os_vector_pop_back(&filter_names) != 0)
				{
					os_vector_free(&names);
					os_vector_free(&filter_names);
					os_free(buff);
					buff = NULL;
					return buff;
				}
			}
			else
			{
				os_vector_push_back(&filter_names, index);
			}
		}
		size = (int32)os_vector_size(&filter_names);
		i = 0;
		int j;
		for (j = 0; j < size; j++)
		{
			buff[i] = '/';
			i++;
			int32 *index = (int32 *)os_vector_at(&filter_names, j);
			int32 k;
			for (k = 0; ; i++, k++)
			{
				if (buff[*index + k] != '\0')
				{
					buff[i] = buff[*index + k];
				}
				else
				{
					break;
				}
			}
		}
		if (0 == i)
		{
			os_str_cpy(buff, "/", path_len + 1);
		}
		else
		{
			buff[i] = '\0';
		}
		os_vector_free(&names);
		os_vector_free(&filter_names);
	}

	return buff;
}

static uint32 get_file_name(char *dest, const char *path, uint32 *index)
{
	uint32 ret = 1;
	if ('/' == path[*index])
	{
		uint32 i = 0;
		ret = 0;
		(*index)++;
		for (; path[*index] != '\0'; (*index)++)
		{
			if (path[*index] != '/')
			{
				break;
			}
		}
		for (; path[*index] != '\0'; (*index)++, i++)
		{
			if (path[*index] == '/')
			{
				break;
			}
			else
			{
				dest[i] = path[*index];
			}
		}
		dest[i] = '\0';
		if (os_str_cmp(dest, "") == 0)
		{
			ret = 1;
		}
	}
	return ret;
}

static void os_file_info_init(os_file_info *finfo)
{
	finfo->name[0] = '\0';
	finfo->size = 0;
	finfo->create_time = os_sys_time();
	finfo->modif_time = os_sys_time();
	finfo->creator = 0;
	finfo->modifier = 0;
	finfo->property = 0x000007b6;
	finfo->files = NULL;
	finfo->dev = NULL;
	finfo->arg = NULL;
	finfo->file_operators = NULL;
	finfo->fs_operators = NULL;
}

void os_vfs_init()
{
	os_file_info_init(&_os_vfs.root);
	os_str_cpy(_os_vfs.root.name, "/", MAX_FILE_NAME_SIZE);
	_os_vfs.root.files = NULL;
	os_create_vfile(DEV_PATH, NULL, NULL);
	os_create_vfile(MNT_PATH, NULL, NULL);
}

void os_vfs_uninit()
{
	if (_os_vfs.root.files != NULL)
	{
		os_map_iterator *itr = os_map_begin(_os_vfs.root.files);
		for (; itr != NULL; itr = os_map_next(_os_vfs.root.files, itr))
		{
			os_file_info *finfo = *(os_file_info **)os_map_second(_os_vfs.root.files, itr);
			os_free(finfo);
		}
		os_map_free(_os_vfs.root.files);
		os_free(_os_vfs.root.files);
		_os_vfs.root.files = NULL;
	}
}

os_file_info *os_create_vfile(const char *path, const os_file_operators *file_operators, const os_fs_operators *fs_operators)
{
	os_file_info *finfo = NULL;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		char name[MAX_FILE_NAME_SIZE];
		uint32 index = 0;
		uint32 child_index = 0;
		uint32 end_index = 0;
		for (; get_file_name(name, pre_path, &index) == 0; )
		{
			child_index = end_index;
			end_index = index;
		}
		if (end_index != 0)
		{
			uint32 flag = 1;
			os_file_info *cur = &_os_vfs.root;
			for (index = 0; index != child_index; )
			{
				os_mem_set(name, 0, MAX_FILE_NAME_SIZE);
				get_file_name(name, pre_path, &index);
				if (cur->files != NULL)
				{
					os_map_iterator *itr = os_map_find(cur->files, name);
					if (itr != NULL)
					{
						cur = *(os_file_info **)os_map_second(cur->files, itr);
					}
					else
					{
						flag = 0;
						break;
					}
				}
				else
				{
					flag = 0;
					break;
				}
			}
			if (flag)
			{
				os_mem_set(name, 0, MAX_FILE_NAME_SIZE);
				get_file_name(name, pre_path, &child_index);
				if (os_str_cmp(name, ".") != 0 && os_str_cmp(name, "..") != 0 && os_str_find(name, "/") == -1 && os_str_find(name, "\\") == -1
					&& os_str_find(name, ":") == -1 && os_str_find(name, "*") == -1 && os_str_find(name, "?") == -1 && os_str_find(name, "\"") == -1
					&& os_str_find(name, "<") == -1 && os_str_find(name, ">") == -1 && os_str_find(name, "|") == -1)
				{
					finfo = (os_file_info *)os_malloc(sizeof(os_file_info));
					os_file_info_init(finfo);
					finfo->file_operators = file_operators;
					finfo->fs_operators = fs_operators;
					os_str_cpy(finfo->name, name, MAX_FILE_NAME_SIZE);
					if (NULL == cur->files)
					{
						cur->files = (os_map *)os_malloc(sizeof(os_map));
						os_map_init(cur->files, MAX_FILE_NAME_SIZE, sizeof(os_file_info *));
					}
					if (os_map_insert(cur->files, name, &finfo) != 0)
					{
						os_free(finfo);
						finfo = NULL;
					}
				}
			}
		}
		os_free(pre_path);
	}
	return finfo;
}

uint32 os_delete_vfile(const char *path)
{
	uint32 ret = 1;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		char name[MAX_FILE_NAME_SIZE];
		uint32 index = 0;
		uint32 child_index = 0;
		uint32 end_index = 0;
		for (; get_file_name(name, pre_path, &index) == 0; )
		{
			child_index = end_index;
			end_index = index;
		}
		if (end_index != 0)
		{
			uint32 flag = 1;
			os_file_info *cur = &_os_vfs.root;
			for (index = 0; index != child_index; )
			{
				os_mem_set(name, 0, MAX_FILE_NAME_SIZE);
				get_file_name(name, pre_path, &index);
				if (cur->files != NULL)
				{
					os_map_iterator *itr = os_map_find(cur->files, name);
					if (itr != NULL)
					{
						cur = *(os_file_info **)os_map_second(cur->files, itr);
					}
					else
					{
						flag = 0;
						break;
					}
				}
				else
				{
					flag = 0;
					break;
				}
			}
			if (flag)
			{
				if (cur->files != NULL)
				{
					os_mem_set(name, 0, MAX_FILE_NAME_SIZE);
					get_file_name(name, pre_path, &child_index);
					os_map_iterator *itr = os_map_find(cur->files, name);
					if (itr != NULL)
					{
						os_file_info *child = *(os_file_info **)os_map_second(cur->files, itr);
						if (NULL == child->files)
						{
							ret = 0;
							os_map_erase(cur->files, itr);
							os_free(child);
							if (os_map_empty(cur->files))
							{
								os_free(cur->files);
								cur->files = NULL;
							}
						}
					}
				}
			}
		}
		os_free(pre_path);
	}
	return ret;
}

uint32 os_register_dev(const char *name, const os_file_operators *file_operators)
{
	if (os_str_cmp(name, ".") != 0 && os_str_cmp(name, "..") != 0 && os_str_find(name, "/") == -1 && os_str_find(name, "\\") == -1
		&& os_str_find(name, ":") == -1 && os_str_find(name, "*") == -1 && os_str_find(name, "?") == -1 && os_str_find(name, "\"") == -1
		&& os_str_find(name, "<") == -1 && os_str_find(name, ">") == -1 && os_str_find(name, "|") == -1)
	{
		char path[MAX_FILE_NAME_SIZE];
		os_str_cpy(path, DEV_PATH, MAX_FILE_NAME_SIZE);
		os_str_cat(path, "/");
		os_str_cat(path, name);
		if (os_create_vfile(path, file_operators, NULL) != NULL)
		{
			return 0;
		}
	}
	return 1;
}

uint32 os_unregister_dev(const char *name)
{
	char path[MAX_FILE_NAME_SIZE];
	os_str_cpy(path, DEV_PATH, MAX_FILE_NAME_SIZE);
	os_str_cat(path, "/");
	os_str_cat(path, name);
	return os_delete_vfile(path);
}

static os_file_info *get_final_vfile(const char *path, uint32 *index)
{
	char name[MAX_FILE_NAME_SIZE];
	os_file_info *cur = &_os_vfs.root;
	uint32 cur_index = *index;
	os_mem_set(name, 0, MAX_FILE_NAME_SIZE);
	for (; get_file_name(name, path, &cur_index) == 0 && cur->files != NULL; )
	{
		os_map_iterator *itr = os_map_find(cur->files, name);
		if (itr != NULL)
		{
			cur = *(os_file_info **)os_map_second(cur->files, itr);
			*index = cur_index;
		}
		else
		{
			break;
		}
		os_mem_set(name, 0, MAX_FILE_NAME_SIZE);
	}
	return cur;
}

uint32 os_mount(const char *mount_name, const char *dev_name, const char *fs_name, uint32 formatting)
{
	if (os_str_cmp(mount_name, ".") != 0 && os_str_cmp(mount_name, "..") != 0 && os_str_find(mount_name, "/") == -1 && os_str_find(mount_name, "\\") == -1
		&& os_str_find(mount_name, ":") == -1 && os_str_find(mount_name, "*") == -1 && os_str_find(mount_name, "?") == -1 && os_str_find(mount_name, "\"") == -1
		&& os_str_find(mount_name, "<") == -1 && os_str_find(mount_name, ">") == -1 && os_str_find(mount_name, "|") == -1)
	{
		char path[MAX_FILE_NAME_SIZE];
		os_str_cpy(path, DEV_PATH, MAX_FILE_NAME_SIZE);
		os_str_cat(path, "/");
		os_str_cat(path, dev_name);
		uint32 index = 0;
		os_file_info *dev_file = get_final_vfile(path, &index);
		if (dev_file != NULL)
		{
			const os_file_system *fs = NULL;
			uint32 i = 0;
			for (i = 0; ; i++)
			{
				fs = _file_systems[i];
				if (fs != NULL)
				{
					if (os_str_cmp(fs->name, fs_name) == 0)
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
			if (fs != NULL)
			{
				os_file_info *mount_file = &_os_vfs.root;
				if (*mount_name != '\0')
				{
					os_str_cpy(path, MNT_PATH, MAX_FILE_NAME_SIZE);
					os_str_cat(path, "/");
					os_str_cat(path, mount_name);
					mount_file = os_create_vfile(path, fs->file_operators, fs->fs_operators);
				}
				else
				{
					mount_file->fs_operators = fs->fs_operators;
					mount_file->file_operators = fs->file_operators;
				}
				if (mount_file != NULL)
				{
					mount_file->dev = dev_file;
					mount_file->fs_operators->mount(mount_file, formatting);
					return 0;
				}
			}
		}
	}
	return 1;
}

uint32 os_unmount(const char *mount_name)
{
	os_file_info *mount_file = &_os_vfs.root;
	char path[MAX_FILE_NAME_SIZE] = "";
	if (*mount_name != '\0')
	{
		os_str_cpy(path, MNT_PATH, MAX_FILE_NAME_SIZE);
		os_str_cat(path, "/");
		os_str_cat(path, mount_name);
		uint32 index = 0;
		mount_file = get_final_vfile(path, &index);
	}
	if (mount_file != NULL && mount_file->fs_operators != NULL && mount_file->fs_operators->unmount != NULL)
	{
		mount_file->fs_operators->unmount(mount_file);
		os_delete_vfile(path);
		return 0;
	}
	return 1;
}

void os_formatting(const char *mount_name)
{
	if (os_str_cmp(mount_name, ".") != 0 && os_str_cmp(mount_name, "..") != 0 && os_str_find(mount_name, "/") == -1 && os_str_find(mount_name, "\\") == -1
		&& os_str_find(mount_name, ":") == -1 && os_str_find(mount_name, "*") == -1 && os_str_find(mount_name, "?") == -1 && os_str_find(mount_name, "\"") == -1
		&& os_str_find(mount_name, "<") == -1 && os_str_find(mount_name, ">") == -1 && os_str_find(mount_name, "|") == -1)
	{
		os_file_info *mount_file = &_os_vfs.root;
		if (*mount_name != '\0')
		{
			char path[MAX_FILE_NAME_SIZE];
			os_str_cpy(path, MNT_PATH, MAX_FILE_NAME_SIZE);
			os_str_cat(path, "/");
			os_str_cat(path, mount_name);
			uint32 index = 0;
			mount_file = get_final_vfile(path, &index);
		}
		if (mount_file != NULL && mount_file->fs_operators != NULL && mount_file->fs_operators->formatting != NULL)
		{
			mount_file->fs_operators->formatting(mount_file);
		}
	}
}

uint32 os_create_dir(const char *path)
{
	uint32 ret = 1;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		uint32 index = 0;
		os_file_info *finfo = get_final_vfile(pre_path, &index);
		if (finfo->fs_operators != NULL && finfo->fs_operators->create_dir != NULL)
		{
			if ('\0' == pre_path[index])
			{
				ret = finfo->fs_operators->create_dir(finfo, "/");
			}
			else
			{
				ret = finfo->fs_operators->create_dir(finfo, &pre_path[index]);
			}
		}
		os_free(pre_path);
	}
	return ret;
}

uint32 os_create_file(const char *path)
{
	uint32 ret = 1;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		uint32 index = 0;
		os_file_info *finfo = get_final_vfile(pre_path, &index);
		if (finfo->fs_operators != NULL && finfo->fs_operators->create_file != NULL)
		{
			if ('\0' == pre_path[index])
			{
				ret = finfo->fs_operators->create_file(finfo, "/");
			}
			else
			{
				ret = finfo->fs_operators->create_file(finfo, &pre_path[index]);
			}
		}
		os_free(pre_path);
	}
	return ret;
}

os_dir *os_open_dir(const char *path)
{
	os_dir *dir = NULL;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		uint32 index = 0;
		dir = (os_dir *)os_malloc(sizeof(os_dir));
		dir->vfile = get_final_vfile(pre_path, &index);
		if (dir->vfile->files != NULL && ('\0' == pre_path[index] || os_str_cmp(pre_path, "/") == 0))
		{
			dir->itr = os_map_begin(dir->vfile->files);
		}
		else
		{
			dir->itr = NULL;
		}
		if (dir->vfile->fs_operators != NULL && dir->vfile->fs_operators->open_dir != NULL)
		{
			if ('\0' == pre_path[index])
			{
				dir->real_dir = dir->vfile->fs_operators->open_dir(dir->vfile, "/");
			}
			else
			{
				dir->real_dir = dir->vfile->fs_operators->open_dir(dir->vfile, &pre_path[index]);
			}
		}
		else
		{
			dir->real_dir = NULL;
		}
		if (NULL == dir->real_dir && pre_path[index] != '\0' && os_str_cmp(pre_path, "/") != 0)
		{
			os_free(dir);
			dir = NULL;
		}
		os_free(pre_path);
	}
	return dir;
}

void os_close_dir(os_dir *dir)
{
	if (dir->real_dir != NULL && dir->vfile->fs_operators != NULL && dir->vfile->fs_operators->close_dir != NULL)
	{
		dir->vfile->fs_operators->close_dir(dir->vfile, dir->real_dir);
	}
	os_free(dir);
}

uint32 os_read_dir(os_file_info *finfo, os_dir *dir)
{
	if (dir->itr != NULL)
	{
		os_file_info *cur_finfo = *(os_file_info **)os_map_second(dir->vfile->files, dir->itr);
		os_str_cpy(finfo->name, cur_finfo->name, MAX_FILE_NAME_SIZE);
		finfo->size = cur_finfo->size;
		finfo->create_time = cur_finfo->create_time;
		finfo->modif_time = cur_finfo->modif_time;
		finfo->creator = cur_finfo->creator;
		finfo->modifier = cur_finfo->modifier;
		finfo->property = cur_finfo->property;
		finfo->files = cur_finfo->files;
		finfo->file_operators = cur_finfo->file_operators;
		finfo->fs_operators = cur_finfo->fs_operators;
		dir->itr = os_map_next(dir->vfile->files, dir->itr);
		return 0;
	}
	else
	{
		if (dir->vfile->fs_operators != NULL && dir->vfile->fs_operators->read_dir != NULL)
		{
			return dir->vfile->fs_operators->read_dir(dir->vfile, finfo, dir->real_dir);
		}
	}
	return 1;
}

uint32 os_delete_dir(const char *path)
{
	uint32 ret = 1;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		uint32 index = 0;
		os_file_info *finfo = get_final_vfile(pre_path, &index);
		if (finfo->fs_operators != NULL && finfo->fs_operators->delete_dir != NULL)
		{
			if ('\0' == pre_path[index])
			{
				ret = finfo->fs_operators->delete_dir(finfo, "/");
			}
			else
			{
				ret = finfo->fs_operators->delete_dir(finfo, &pre_path[index]);
			}
		}
		os_free(pre_path);
	}
	return ret;
}

uint32 os_delete_file(const char *path)
{
	uint32 ret = 1;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		uint32 index = 0;
		os_file_info *finfo = get_final_vfile(pre_path, &index);
		if (finfo->fs_operators != NULL && finfo->fs_operators->delete_file != NULL)
		{
			if ('\0' == pre_path[index])
			{
				finfo->fs_operators->delete_file(finfo, "/");
			}
			else
			{
				finfo->fs_operators->delete_file(finfo, &pre_path[index]);
			}
			ret = 0;
		}
		os_free(pre_path);
	}
	return ret;
}

uint32 os_move_file(const char *dest, const char *src)
{
	return 0;
}

uint32 os_copy_file(const char *dest, const char *src)
{
	return 0;
}

OS_FILE os_open_file(const char *path, uint32 flags)
{
	return NULL;
}

void os_close_file(OS_FILE file)
{

}

uint32 os_read_file(OS_FILE file, void *data, uint32 len)
{
	return 0;
}

uint32 os_write_file(OS_FILE file, void *data, uint32 len)
{
	return 0;
}

uint32 os_seek_file(OS_FILE file, int64 offset, uint32 fromwhere)
{
	return 0;
}

uint64 os_tell_file(OS_FILE file)
{
	return 0;
}

void *os_ioctl(uint32 command, void *arg)
{
	return NULL;
}

uint64 os_total_size(const char *mount_name) 
{
	os_file_info *mount_file = &_os_vfs.root;
	if (*mount_name != '\0')
	{
		char path[MAX_FILE_NAME_SIZE];
		os_str_cpy(path, MNT_PATH, MAX_FILE_NAME_SIZE);
		os_str_cat(path, "/");
		os_str_cat(path, mount_name);
		uint32 index = 0;
		mount_file = get_final_vfile(path, &index);
	}
	if (mount_file != NULL && mount_file->fs_operators != NULL && mount_file->fs_operators->total_size != NULL)
	{
		return mount_file->fs_operators->total_size(mount_file);
	}
	return 0;
}

uint64 os_free_size(const char *mount_name)
{
	os_file_info *mount_file = &_os_vfs.root;
	if (*mount_name != '\0')
	{
		char path[MAX_FILE_NAME_SIZE];
		os_str_cpy(path, MNT_PATH, MAX_FILE_NAME_SIZE);
		os_str_cat(path, "/");
		os_str_cat(path, mount_name);
		uint32 index = 0;
		mount_file = get_final_vfile(path, &index);
	}
	if (mount_file != NULL && mount_file->fs_operators != NULL && mount_file->fs_operators->free_size != NULL)
	{
		return mount_file->fs_operators->free_size(mount_file);
	}
	return 0;
}