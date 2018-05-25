#include "os_fs.h"
#include "os_journal.h"
#include "base/os_string.h"
#include "vtos.h"
static const char JOURNAL_PATH[] = "/.journal";
void journal_create(os_journal *journal)
{
	/*journal->enable = 2;
	journal->create_file(JOURNAL_PATH);
	file_obj *file = open_file(JOURNAL_PATH, FS_WRITE);
	if (file != NULL)
	{
		journal->write_file(file, NULL, JOURNAL_FILE_SIZE);
		close_file(file);
	}
	journal_reset(journal);
	journal->enable = 0;*/
}

void os_journal_init(os_journal *journal, create_file_callback arg1, write_file_callback arg2, os_cluster *cluster)
{
	/*journal->create_file = arg1;
	journal->write_file = arg2;
	journal->enable = 0;
	journal->buff = (uint32 *)os_malloc(FS_CLUSTER_SIZE);
	os_mem_set(journal->buff, 0, FS_CLUSTER_SIZE);
	journal->file = NULL;
	journal->index = 0;
	journal->cluster = cluster;*/
}

void os_journal_uninit(os_journal *journal)
{
	/*journal->create_file = NULL;
	journal->write_file = NULL;
	journal->enable = 0;
	os_free(journal->buff);
	journal->buff = NULL;
	journal->file = NULL;
	journal->index = 0;
	journal->cluster = NULL;*/
}

void journal_start(os_journal *journal)
{
	/*if (0 == journal->enable)
	{
		journal->file = open_file(JOURNAL_PATH, FS_WRITE);
		if (journal->file != NULL)
		{
			journal->enable = 1;
			journal->index = 0;
		}
	}*/
}

void journal_end(os_journal *journal)
{
	/*if (1 == journal->enable)
	{
		if (journal->file != NULL)
		{
			close_file((file_obj *)journal->file);
			journal->file = NULL;
		}
		journal->enable = 0;
		if (journal->buff[0] != 0)
		{
			journal_reset(journal);
		}
	}*/
}

void journal_write(uint32 id, os_journal *journal)
{
	/*if (1 == journal->enable)
	{
		journal->enable = 0;
		seek_file(journal->file, FS_CLUSTER_SIZE + journal->index * FS_CLUSTER_SIZE, FS_SEEK_SET);
		void *data = os_malloc(FS_CLUSTER_SIZE);
		cluster_read(id, data, journal->cluster);
		journal->write_file(journal->file, data, FS_CLUSTER_SIZE);
		os_free(data);
		if (is_little_endian())
		{
			journal->buff[journal->index] = id;
		}
		else
		{
			journal->buff[journal->index] = convert_endian(id);
		}
		seek_file(journal->file, 0, FS_SEEK_SET);
		journal->write_file(journal->file, journal->buff, FS_CLUSTER_SIZE);
		journal->index++;
		journal->enable = 1;
	}*/
}

void journal_reset(os_journal *journal)
{
	/*file_obj *file = open_file(JOURNAL_PATH, FS_WRITE);
	if (file != NULL)
	{
		os_mem_set(journal->buff, 0, FS_CLUSTER_SIZE);
		journal->write_file(file, journal->buff, FS_CLUSTER_SIZE);
		close_file(file);
	}*/
}

uint32 restore_from_journal(os_journal *journal)
{
	/*file_obj *file = open_file(JOURNAL_PATH, FS_READ);
	if (file != NULL)
	{
		read_file(file, journal->buff, FS_CLUSTER_SIZE);
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
				seek_file(file, FS_CLUSTER_SIZE + i * FS_CLUSTER_SIZE, FS_SEEK_SET);
				read_file(file, buff, FS_CLUSTER_SIZE);
				cluster_write(journal->buff[i], buff, journal->cluster);
				os_free(buff);
			}
			journal_reset(journal);
			close_file(file);
			return 0;
		}
		close_file(file);
	}*/
	return 1;
}
