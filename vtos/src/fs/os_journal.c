#include "fs/os_fs.h"
#include "fs/os_journal.h"
#include "base/os_string.h"
#include <stdlib.h>
static const char *FILES_PATH[] = {"/.journal", "/.journal_backup"};
static uint32 _file_index = 0;
static create_file_callback _create_file = NULL;
static delete_file_callback _delete_file = NULL;
static write_file_callback _write_file = NULL;
static move_file_callback _move_file = NULL;

static void create_journal(const char *path)
{
	_create_file(path);
	file_obj *file = open_file(path, FS_WRITE);
	if (file != NULL)
	{
		uint32 *buff = (uint32 *)malloc(FS_CLUSTER_SIZE);
		os_str_cpy((char *)buff, "journal", FS_CLUSTER_SIZE);
		buff[2] = 0;
		_write_file(file, buff, FS_CLUSTER_SIZE);
		_write_file(file, NULL, JOURNAL_FILE_SIZE - FS_CLUSTER_SIZE);
		free(buff);
		close_file(file);
	}
}

static void update_journal_file(uint32 index)
{
	_file_index = index;
	if (1 == _file_index)
	{
		_delete_file(FILES_PATH[0]);
		create_journal(FILES_PATH[0]);
	}
	else
	{
		_delete_file(FILES_PATH[1]);
		create_journal(FILES_PATH[1]);
	}
}

void journal_init()
{
	create_journal(FILES_PATH[0]);
	create_journal(FILES_PATH[1]);
}

void register_callback(create_file_callback arg1, delete_file_callback arg2, write_file_callback arg3, move_file_callback arg4)
{
	_create_file = arg1;
	_delete_file = arg2;
	_write_file = arg3;
	_move_file = arg4;
}