#ifndef __OS_JOURNAL_H__
#define __OS_JOURNAL_H__
#include "fs/os_fs_def.h"
#define JOURNAL_FILE_SIZE (FS_CLUSTER_SIZE * 1024)
typedef uint32(*create_file_callback)(const char *path);
typedef uint32 (*write_file_callback)(void *file, void *data, uint32 len);
typedef uint32(*read_file_callback)(void *file, void *data, uint32 len);
struct os_journal
{
	uint32 enable;
	create_file_callback create_file;
	write_file_callback write_file;
	read_file_callback read_file;
	uint32 *buff;
};
void os_journal_init(create_file_callback arg1, write_file_callback arg2, read_file_callback arg3);
void os_journal_uninit();
void journal_start();
void journal_end();
void journal_write(uint32 id, void *data);
void journal_create();
void journal_reset();
void restore_from_journal();
#endif