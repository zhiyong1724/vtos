#include "os_vfs.h"
#include "base/os_string.h"
#include "vtos.h"
#include "base/os_vector.h"
static struct os_vfs _os_vfs;

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
		buff[i] = '\0';
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

static void os_file_operators_init(os_file_operators *file_operators)
{
	file_operators->module_init = NULL;
	file_operators->module_uninit = NULL;
	file_operators->os_ioctl = NULL;
	file_operators->open_file = NULL;
	file_operators->close_file = NULL;
	file_operators->read_file = NULL;
	file_operators->write_file = NULL;
	file_operators->seek_file = NULL;
	file_operators->tell_file = NULL;
}

static void os_fs_operators_init(os_fs_operators *fs_operators)
{
	fs_operators->mount = NULL;
	fs_operators->unmount = NULL;
	fs_operators->create_dir = NULL;
	fs_operators->create_file = NULL;
	fs_operators->open_dir = NULL;
	fs_operators->close_dir = NULL;
	fs_operators->read_dir = NULL;
	fs_operators->delete_dir = NULL;
	fs_operators->delete_file = NULL;
	fs_operators->move_file = NULL;
	fs_operators->copy_file = NULL;
}

static void os_file_info_init(os_file_info *finfo)
{
	finfo->name[0] = '\0';
	_os_vfs.root.size = 0;
	_os_vfs.root.create_time = os_sys_time();
	_os_vfs.root.modif_time = os_sys_time();
	_os_vfs.root.creator = 0;
	_os_vfs.root.modifier = 0;
	_os_vfs.root.property = 0x000007b6;
	_os_vfs.root.files = NULL;
	_os_vfs.root.file_operators = NULL;
	_os_vfs.root.fs_operators = NULL;
	_os_vfs.root.real_dir = NULL;
}

void os_vfs_init()
{
	os_file_info_init(&_os_vfs.root);
	os_str_cpy(_os_vfs.root.name, "/", MAX_FILE_NAME_SIZE);
	_os_vfs.root.files = (os_map *)os_malloc(sizeof(os_map));
	os_map_init(_os_vfs.root.files, MAX_FILE_NAME_SIZE, sizeof(os_file_info));
}

void os_vfs_uninit()
{
	os_free(_os_vfs.root.files);
	if (_os_vfs.root.file_operators != NULL)
	{
		os_free(_os_vfs.root.file_operators);
		_os_vfs.root.file_operators = NULL;
	}
	if (_os_vfs.root.fs_operators != NULL)
	{
		os_free(_os_vfs.root.fs_operators);
		_os_vfs.root.fs_operators = NULL;
	}
}

uint32 os_create_vfile(const char *path)
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
			os_file_info *cur = &_os_vfs.root;
			for (index = 0; index != child_index; )
			{
				get_file_name(name, pre_path, &index);
				if (os_str_cmp(name, ".") != 0 && os_str_cmp(name, "..") != 0 && os_str_find(name, "/") == -1 && os_str_find(name, "\\") == -1
					&& os_str_find(name, ":") == -1 && os_str_find(name, "*") == -1 && os_str_find(name, "?") == -1 && os_str_find(name, "\"") == -1
					&& os_str_find(name, "<") == -1 && os_str_find(name, ">") == -1 && os_str_find(name, "|") == -1)
				{
					if (cur->files != NULL)
					{
						os_map_iterator *itr = os_map_find(cur->files, name);
						if (itr != NULL)
						{
							cur = os_map_second(cur->files, itr);
						}
						else
						{
							break;
						}
					}
				}
			}
			if (index == child_index)
			{
				get_file_name(name, pre_path, &child_index);
				os_file_info *finfo = (os_file_info *)os_malloc(sizeof(os_file_info));
				os_file_info_init(finfo);
				os_str_cpy(finfo->name, name, MAX_FILE_NAME_SIZE);
				if (NULL == cur->files)
				{
					cur->files = (os_map *)os_malloc(sizeof(os_map));
					os_map_init(cur->files, MAX_FILE_NAME_SIZE, sizeof(os_file_info));
				}
				ret = os_map_insert(cur->files, name, finfo);
				os_free(finfo);
			}
		}
		os_free(pre_path);
	}
	return ret;
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
			os_file_info *cur = &_os_vfs.root;
			for (index = 0; index != child_index; )
			{
				get_file_name(name, pre_path, &index);
				if (cur->files != NULL)
				{
					os_map_iterator *itr = os_map_find(cur->files, name);
					if (itr != NULL)
					{
						cur = os_map_second(cur->files, itr);
					}
					else
					{
						break;
					}
				}
			}
			if (index == child_index)
			{
				if (cur->files != NULL)
				{
					get_file_name(name, pre_path, &child_index);
					if (os_str_cmp(name, ".") != 0 && os_str_cmp(name, "..") != 0 && os_str_find(name, "/") == -1 && os_str_find(name, "\\") == -1
						&& os_str_find(name, ":") == -1 && os_str_find(name, "*") == -1 && os_str_find(name, "?") == -1 && os_str_find(name, "\"") == -1
						&& os_str_find(name, "<") == -1 && os_str_find(name, ">") == -1 && os_str_find(name, "|") == -1)
					{
						os_map_iterator *itr = os_map_find(cur->files, name);
						if (itr != NULL)
						{
							os_file_info *child = os_map_second(cur->files, itr);
							if (NULL == child->files)
							{
								ret = 0;
								os_map_erase(cur->files, itr);
								if (os_map_empty(cur->files))
								{
									os_map_free(cur->files);
									cur->files = NULL;
								}
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

void os_module_init()
{

}

void os_module_uninit()
{

}

void os_mount()
{

}

void os_unmount()
{

}

uint32 os_create_dir(const char *path)
{
	return 0;
}

uint32 os_create_file(const char *path)
{
	return 0;
}

OS_DIR os_open_dir(const char *path)
{
	return NULL;
}

void os_close_dir(OS_DIR dir)
{

}

uint32 os_read_dir(os_file_info *finfo, OS_DIR dir)
{
	return 0;
}

uint32 os_delete_dir(const char *path)
{
	return 0;
}

uint32 os_delete_file(const char *path)
{
	return 0;
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