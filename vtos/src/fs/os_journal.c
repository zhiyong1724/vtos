#include "os_fs.h"
#include "os_journal.h"
#include "base/os_string.h"
#include "os_cluster.h"
#include "vtos.h"
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

void os_journal_init(create_file_callback arg1, write_file_callback arg2)
{
	_os_journal.create_file = arg1;
	_os_journal.write_file = arg2;
	_os_journal.enable = 0;
	_os_journal.buff = (uint32 *)os_malloc(FS_CLUSTER_SIZE);
	os_mem_set(_os_journal.buff, 0, FS_CLUSTER_SIZE);
	_os_journal.file = NULL;
	_os_journal.index = 0;
}

void os_journal_uninit()
{
	_os_journal.create_file = NULL;
	_os_journal.write_file = NULL;
	_os_journal.enable = 0;
	os_free(_os_journal.buff);
	_os_journal.buff = NULL;
	_os_journal.file = NULL;
	_os_journal.index = 0;
}

void journal_start()
{
	if (0 == _os_journal.enable)
	{
		_os_journal.file = open_file(JOURNAL_PATH, FS_WRITE);
		if (_os_journal.file != NULL)
		{
			_os_journal.enable = 1;
			_os_journal.index = 0;
		}
	}
}

void journal_end()
{
	if (1 == _os_journal.enable)
	{
		if (_os_journal.file != NULL)
		{
			close_file((file_obj *)_os_journal.file);
			_os_journal.file = NULL;
		}
		_os_journal.enable = 0;
		if (_os_journal.buff[0] != 0)
		{
			journal_reset();
		}
	}
}

void journal_write(uint32 id)
{
	if (1 == _os_journal.enable)
	{
		_os_journal.enable = 0;
		seek_file(_os_journal.file, FS_CLUSTER_SIZE + _os_journal.index * FS_CLUSTER_SIZE, FS_SEEK_SET);
		void *data = os_malloc(FS_CLUSTER_SIZE);
		cluster_read(id, data);
		_os_journal.write_file(_os_journal.file, data, FS_CLUSTER_SIZE);
		os_free(data);
		if (is_little_endian())
		{
			_os_journal.buff[_os_journal.index] = id;
		}
		else
		{
			_os_journal.buff[_os_journal.index] = convert_endian(id);
		}
		seek_file(_os_journal.file, 0, FS_SEEK_SET);
		_os_journal.write_file(_os_journal.file, _os_journal.buff, FS_CLUSTER_SIZE);
		_os_journal.index++;
		_os_journal.enable = 1;
	}
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

uint32 restore_from_journal()
{
	file_obj *file = open_file(JOURNAL_PATH, FS_READ);
	if (file != NULL)
	{
		read_file(file, _os_journal.buff, FS_CLUSTER_SIZE);
		int32 i = 0;
		for (i = 0; _os_journal.buff[i] != 0; i++);
		if (i > 0)
		{
			i--;
			if (!is_little_endian())
			{
				int32 j;
				for (j = i; j >= 0; j--)
				{
					_os_journal.buff[j] = convert_endian(_os_journal.buff[j]);
				}
			}
			for (; i >= 0; i--)
			{
				void *buff = os_malloc(FS_CLUSTER_SIZE);
				seek_file(file, FS_CLUSTER_SIZE + i * FS_CLUSTER_SIZE, FS_SEEK_SET);
				read_file(file, buff, FS_CLUSTER_SIZE);
				cluster_write(_os_journal.buff[i], buff);
				os_free(buff);
			}
			journal_reset();
			close_file(file);
			return 0;
		}
		close_file(file);
	}
	return 1;
}
