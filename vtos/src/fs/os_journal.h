#ifndef __OS_JOURNAL_H__
#define __OS_JOURNAL_H__
#include "fs/os_fs_def.h"
#define JOURNAL_FILE_SIZE (FS_CLUSTER_SIZE / 8 * FS_CLUSTER_SIZE)
typedef uint32(*create_file_callback)(const char *path);
typedef uint32(*delete_file_callback)(const char *path);
typedef uint32 (*write_file_callback)(void *file, void *data, uint32 len);
typedef uint32(*read_file_callback)(void *file, void *data, uint32 len);
struct os_journal
{
	uint32 enable;
	uint32 file_index;
	create_file_callback create_file;
	delete_file_callback delete_file;
	write_file_callback write_file;
	read_file_callback read_file;
};
void journal_init();
void journal_start();
void journal_end();
void journal_write(uint32 id);
void register_callback(create_file_callback arg1, delete_file_callback arg2, write_file_callback arg3, read_file_callback arg4);
#endif