#include "os_fs.h"
#include "os_journal.h"
#include "base/os_string.h"
#include "vtos.h"
static const char JOURNAL_PATH[] = "/.journal";
void journal_create(os_journal *journal)
{
	journal->enable = 2;
	journal->create_file(JOURNAL_PATH, (os_fs *)journal->fs);
	file_obj *file = fs_open_file(JOURNAL_PATH, FS_WRITE, (os_fs *)journal->fs);
	if (file != NULL)
	{
		journal->write_file(file, NULL, JOURNAL_FILE_SIZE, (os_fs *)journal->fs);
		fs_close_file(file, (os_fs *)journal->fs);
	}
	journal_reset(journal);
	journal->enable = 0;
}

void os_journal_init(os_journal *journal, create_file_callback create_file, write_file_callback write_file, read_file_callback read_file, void *fs)
{
	journal->create_file = create_file;
	journal->write_file = write_file;
	journal->read_file = read_file;
	journal->enable = 0;
	journal->buff = (uint32 *)os_malloc(FS_CLUSTER_SIZE);
	os_mem_set(journal->buff, 0, FS_CLUSTER_SIZE);
	journal->file = NULL;
	journal->index = 0;
	journal->fs = fs;
}

void os_journal_uninit(os_journal *journal)
{
	journal->create_file = NULL;
	journal->write_file = NULL;
	journal->read_file = NULL;
	journal->enable = 0;
	os_free(journal->buff);
	journal->buff = NULL;
	journal->file = NULL;
	journal->index = 0;
	journal->fs = NULL;
}

void journal_start(os_journal *journal)
{
	if (0 == journal->enable)
	{
		journal->file = fs_open_file(JOURNAL_PATH, FS_WRITE, (os_fs *)journal->fs);
		if (journal->file != NULL)
		{
			journal->enable = 1;
			journal->index = 0;
		}
	}
}

void journal_end(os_journal *journal)
{
	if (1 == journal->enable)
	{
		if (journal->file != NULL)
		{
			fs_close_file((file_obj *)journal->file, (os_fs *)journal->fs);
			journal->file = NULL;
		}
		journal->enable = 0;
		if (journal->buff[0] != 0)
		{
			journal_reset(journal);
		}
	}
}

void journal_write(uint32 id, os_journal *journal)
{
	if (1 == journal->enable)
	{
		journal->enable = 0;
		fs_seek_file(journal->file, FS_CLUSTER_SIZE + journal->index * FS_CLUSTER_SIZE, FS_SEEK_SET, (os_fs *)journal->fs);
		void *data = os_malloc(FS_CLUSTER_SIZE);
		cluster_read(id, data, &((os_fs *)journal->fs)->cluster);
		journal->write_file(journal->file, data, FS_CLUSTER_SIZE, (os_fs *)journal->fs);
		os_free(data);
		if (is_little_endian())
		{
			journal->buff[journal->index] = id;
		}
		else
		{
			journal->buff[journal->index] = convert_endian(id);
		}
		fs_seek_file(journal->file, 0, FS_SEEK_SET, (os_fs *)journal->fs);
		journal->write_file(journal->file, journal->buff, FS_CLUSTER_SIZE, (os_fs *)journal->fs);
		journal->index++;
		journal->enable = 1;
	}
}

void journal_reset(os_journal *journal)
{
	file_obj *file = fs_open_file(JOURNAL_PATH, FS_WRITE, (os_fs *)journal->fs);
	if (file != NULL)
	{
		os_mem_set(journal->buff, 0, FS_CLUSTER_SIZE);
		journal->write_file(file, journal->buff, FS_CLUSTER_SIZE, (os_fs *)journal->fs);
		fs_close_file(file, (os_fs *)journal->fs);
	}
}

uint32 restore_from_journal(os_journal *journal)
{
	file_obj *file = fs_open_file(JOURNAL_PATH, FS_READ, (os_fs *)journal->fs);
	if (file != NULL)
	{
		journal->read_file(file, journal->buff, FS_CLUSTER_SIZE, (os_fs *)journal->fs);
		int32 i = 0;
		for (i = 0; journal->buff[i] != 0; i++);
		if (i > 0)
		{
			i--;
			if (!is_little_endian())
			{
				int32 j;
				for (j = i; j >= 0; j--)
				{
					journal->buff[j] = convert_endian(journal->buff[j]);
				}
			}
			for (; i >= 0; i--)
			{
				void *buff = os_malloc(FS_CLUSTER_SIZE);
				fs_seek_file(file, FS_CLUSTER_SIZE + i * FS_CLUSTER_SIZE, FS_SEEK_SET, (os_fs *)journal->fs);
				journal->read_file(file, buff, FS_CLUSTER_SIZE, (os_fs *)journal->fs);
				cluster_write(journal->buff[i], buff, &((os_fs *)journal->fs)->cluster);
				os_free(buff);
			}
			journal_reset(journal);
			fs_close_file(file, (os_fs *)journal->fs);
			return 0;
		}
		fs_close_file(file, (os_fs *)journal->fs);
	}
	return 1;
}
