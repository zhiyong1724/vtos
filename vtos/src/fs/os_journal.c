#include "fs/os_fs.h"
#include "fs/os_journal.h"
#include "base/os_string.h"
#include <stdlib.h>
static const char *FILES_PATH[] = {"/.journal", "/.journal_backup"};
static struct os_journal _os_journal;

static void create_journal(const char *path)
{
	_os_journal.create_file(path);
	file_obj *file = open_file(path, FS_WRITE);
	if (file != NULL)
	{
		uint32 *buff = (uint32 *)malloc(FS_CLUSTER_SIZE);
		os_str_cpy((char *)buff, "journal", FS_CLUSTER_SIZE);
		buff[2] = 0;
		_os_journal.write_file(file, buff, FS_CLUSTER_SIZE);
		_os_journal.write_file(file, NULL, JOURNAL_FILE_SIZE - FS_CLUSTER_SIZE);
		free(buff);
		close_file(file);
	}
}

static void update_journal_file(uint32 index)
{
	_os_journal.file_index = index;
	if (1 == _os_journal.file_index)
	{
		_os_journal.delete_file(FILES_PATH[0]);
		create_journal(FILES_PATH[0]);
	}
	else
	{
		_os_journal.delete_file(FILES_PATH[1]);
		create_journal(FILES_PATH[1]);
	}
}

void journal_init()
{
	_os_journal.file_index = 0;
	create_journal(FILES_PATH[0]);
	create_journal(FILES_PATH[1]);
}

void register_callback(create_file_callback arg1, delete_file_callback arg2, write_file_callback arg3, read_file_callback arg4)
{
	_os_journal.create_file = arg1;
	_os_journal.delete_file = arg2;
	_os_journal.write_file = arg3;
	_os_journal.read_file = arg4;
}