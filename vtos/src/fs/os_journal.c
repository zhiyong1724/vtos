#include "fs/os_fs.h"
#include "fs/os_journal.h"
#include "base/os_string.h"
#include <stdlib.h>
static struct os_journal _os_journal;
static const char JOURNAL_PATH[] = "/.journal";
void journal_create()
{
	_os_journal.enable = 2;
	_os_journal.create_file(JOURNAL_PATH);
	file_obj *file = open_file(JOURNAL_PATH, FS_WRITE);
	if (file != NULL)
	{
		_os_journal.write_file(file, NULL, JOURNAL_FILE_SIZE);
		close_file(file);
	}
	journal_reset();
	_os_journal.enable = 0;
}

void os_journal_init(create_file_callback arg1, write_file_callback arg2, read_file_callback arg3)
{
	_os_journal.create_file = arg1;
	_os_journal.write_file = arg2;
	_os_journal.read_file = arg3;
	_os_journal.enable = 0;
	_os_journal.buff = (uint32 *)malloc(FS_CLUSTER_SIZE);
}

void os_journal_uninit()
{
	_os_journal.create_file = NULL;
	_os_journal.write_file = NULL;
	_os_journal.read_file = NULL;
	_os_journal.enable = 0;
	free(_os_journal.buff);
	_os_journal.buff = NULL;
}

void journal_start()
{
	if (0 == _os_journal.enable)
	{
		_os_journal.enable = 1;
	}
}

void journal_end()
{
	_os_journal.enable = 0;
}

void journal_write(uint32 id, void *data)
{

}

void journal_reset()
{
	file_obj *file = open_file(JOURNAL_PATH, FS_WRITE);
	if (file != NULL)
	{
		os_mem_set(_os_journal.buff, 0, FS_CLUSTER_SIZE);
		_os_journal.write_file(file, _os_journal.buff, FS_CLUSTER_SIZE);
		close_file(file);
	}
}

void restore_from_journal()
{

}
