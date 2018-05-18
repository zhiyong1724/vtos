#include "os_vfs.h"
#include "base/os_string.h"
#include "vtos.h"
#include "base/os_vector.h"
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