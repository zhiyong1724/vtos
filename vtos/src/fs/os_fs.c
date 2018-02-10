#include "fs/os_fs.h"
#include "lib/os_string.h"
#include "fs/os_cluster.h"
#include <stdio.h>
#include <vtos.h>
#include <stdlib.h>
static super_cluster *_super = NULL;
static fnode *_root = NULL;
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

static void super_cluster_init(super_cluster *super)
{
	super->flag = 0xaa55a55a;
	os_str_cpy(super->name, "emfs", 16);
	super->root_id = ROOT_CLUSTER_ID;
	super->backup_id = 0;
}

static void file_info_init(file_info *info)
{
	info->cluster_id = 0;
	info->size = 0;
	info->cluster_count = 0;
	info->create_time = os_get_time();
	info->modif_time = os_get_time();
	info->creator = 0;
	info->modifier = 0;
	info->property = 0x000001ff; //.10 dir or file;.9 sys w;.876 user r,w,x;.543 group r,w,x;.210 other r,w,x
	info->file_count = 2;
	info->name[0] = '\0';
}

static void super_cluster_flush()
{
	if (is_little_endian())
	{
		cluster_write(SUPER_CLUSTER_ID, (uint8 *)_super);
	}
	else
	{
		super_cluster *temp = (super_cluster *)malloc(FS_PAGE_SIZE);
		os_mem_cpy(temp, _super, FS_PAGE_SIZE);

		temp->flag = convert_endian(_super->flag);
		temp->root_id = convert_endian(_super->root_id);
		temp->backup_id = convert_endian(_super->backup_id);
		cluster_write(SUPER_CLUSTER_ID, (uint8 *)temp);
		free(temp);
	}
}

static void super_cluster_load()
{
	if (is_little_endian())
	{
		cluster_read(SUPER_CLUSTER_ID, (uint8 *)_super);
	}
	else
	{
		cluster_read(SUPER_CLUSTER_ID, (uint8 *)_super);
		_super->flag = convert_endian(_super->flag);
		_super->root_id = convert_endian(_super->root_id);
		_super->backup_id = convert_endian(_super->backup_id);
	}
}

static void file_info_flush(uint32 id, file_info *finfo)
{
	if (is_little_endian())
	{
		cluster_write(id, (uint8 *)finfo);
	}
	else
	{
		file_info *temp = (file_info  *)malloc(FS_PAGE_SIZE);
		temp->cluster_id = convert_endian(finfo->cluster_id);
		temp->cluster_count = convert_endian(finfo->cluster_count);
		temp->creator = convert_endian(finfo->creator);
		temp->modifier = convert_endian(finfo->modifier);
		temp->property = convert_endian(finfo->property);
		temp->file_count = convert_endian(finfo->file_count);
		temp->size = convert_endian64(finfo->size);
		temp->create_time = convert_endian64(finfo->create_time);
		temp->modif_time = convert_endian64(finfo->modif_time);
		cluster_write(id, (uint8 *)temp);
		free(temp);
	}
}

static file_info *file_info_load(uint32 id)
{
	file_info *finfo = (file_info *)malloc(FS_PAGE_SIZE);
	cluster_read(id, (uint8 *)finfo);
	if (!is_little_endian())
	{
		finfo->cluster_id = convert_endian(finfo->cluster_id);
		finfo->cluster_count = convert_endian(finfo->cluster_count);
		finfo->creator = convert_endian(finfo->creator);
		finfo->modifier = convert_endian(finfo->modifier);
		finfo->property = convert_endian(finfo->property);
		finfo->file_count = convert_endian(finfo->file_count);
		finfo->size = convert_endian64(finfo->size);
		finfo->create_time = convert_endian64(finfo->create_time);
		finfo->modif_time = convert_endian64(finfo->modif_time);
	}
	return finfo;
}

void fs_formatting()
{
	file_info *finfo = NULL;
	while (sizeof(fnode) != FS_PAGE_SIZE);
	cluster_controler_init();
	cluster_manager_init();
	_super = (super_cluster *)malloc(FS_PAGE_SIZE);
	super_cluster_init(_super);
	finfo = (file_info *)malloc(FS_PAGE_SIZE);
	file_info_init(finfo);
	finfo->property |= 0x00000600;
	os_str_cpy(finfo->name, ".", FS_MAX_NAME_SIZE);
	_root = insert_to_btree(_root, finfo);
	os_str_cpy(finfo->name, "..", FS_MAX_NAME_SIZE);
	_root = insert_to_btree(_root, finfo);
	_root->finfo[0].cluster_id = _root->head.node_id;
	_root->finfo[1].cluster_id = _root->head.node_id;
	os_str_cpy(finfo->name, "/", FS_MAX_NAME_SIZE);
	finfo->cluster_id = _root->head.node_id;
	file_info_flush(ROOT_CLUSTER_ID, finfo);
	free(finfo);
	super_cluster_flush();
	fs_unloading();
}

void fs_loading()
{
	file_info *finfo = NULL;
	while (sizeof(fnode) != FS_PAGE_SIZE);
	cluster_controler_init();
	cluster_manager_load();
	_super = (super_cluster *)malloc(FS_PAGE_SIZE);
	super_cluster_load();
	finfo = file_info_load(ROOT_CLUSTER_ID);
	_root = fnode_load(finfo->cluster_id);
	free(finfo);
}

void fs_unloading()
{
	uninit();
	if (_super != NULL)
	{
		free(_super);
		_super = NULL;
	}
	if (_root != NULL)
	{
		free(_root);
		_root = NULL;
	}
}

static fnode *get_partent(const char *path, uint32 *idx, char *child_name, uint32 *is_exist)
{
	uint32 index = 0;
	char parent_name[FS_MAX_NAME_SIZE];
	fnode *cur = _root;
	fnode *ret = _root;
	*is_exist = 0;
	if (get_file_name(parent_name, path, &index) == 0)
	{
		while (get_file_name(child_name, path, &index) == 0)
		{
			if (ret != _root)
			{
				free(ret);
			}
			ret = find_from_tree2(cur, idx, parent_name);
			if (ret != NULL)
			{
				if (ret->finfo[*idx].property & 0x00000400)
				{
					if (cur != ret)
					{
						free(cur);
					}
					cur = fnode_load(ret->finfo[*idx].cluster_id);
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
			os_str_cpy(parent_name, child_name, FS_MAX_NAME_SIZE);
		}
		if (ret->finfo[*idx].property & 0x00000400)
		{
			if (find_from_tree(cur, NULL, child_name) == 0)
			{
				*is_exist = 1;
			}
		}
		
		if (cur != NULL && cur != _root && cur != ret)
		{
			free(cur);
		}
	}
	return ret;
}

static file_info *get_file_info(const char *path)
{
	uint32 flag = 0;
	uint32 index = 0;
	char name[FS_MAX_NAME_SIZE];
	fnode *cur = _root;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	while (get_file_name(name, path, &index) == 0)
	{
		if (find_from_tree(cur, finfo, name) == 0)
		{
			if (finfo->property & 0x00000400)
			{
				if (cur != _root)
				{
					free(cur);
				}
				cur = fnode_load(finfo->cluster_id);
			}
			else
			{
				flag = 1;
				break;
			}
		}
		else
		{
			flag = 1;
			break;
		}
	}

	if (cur != NULL && cur != _root)
	{
		free(cur);
	}
	if (flag)
	{
		free(finfo);
		finfo = NULL;
	}
	return finfo;
}

static dir_ctl *init_dir_ctl(uint32 id)
{
	dir_ctl *dir = (dir_ctl *)malloc(sizeof(dir_ctl));
	dir->cur = NULL;
	dir->id = id;
	dir->index = 0;
	dir->parent = NULL;
	return dir;
}

static void insert_to_root(file_info *child)
{
	fnode *root = insert_to_btree(_root, child);
	if (root != _root)
	{
		file_info *finfo = file_info_load(_super->root_id);
		_root = root;
		finfo->cluster_id = _root->head.node_id;
		file_info_flush(_super->root_id, finfo);
		free(finfo);
	}
}

static void remove_from_root(file_info *child)
{
	fnode *root = remove_from_btree(_root, child->name);
	if (root != _root)
	{
		file_info *finfo = file_info_load(_super->root_id);
		_root = root;
		finfo->cluster_id = _root->head.node_id;
		file_info_flush(_super->root_id, finfo);
		free(finfo);
	}

}

static void insert_to_parent(file_info *parent, file_info *child)
{
	fnode *root = fnode_load(parent->cluster_id);
	root = insert_to_btree(root, child);
	parent->file_count++;
	parent->cluster_id = root->head.node_id;
	free(root);
}

static void remove_from_parent(file_info *parent, file_info *child)
{
	fnode *root = fnode_load(parent->cluster_id);
	root = remove_from_btree(root, child->name);
	parent->file_count--;
	parent->cluster_id = root->head.node_id;
	free(root);
}

static uint32 do_create_file(const char *path, file_info *finfo)
{
	uint32 ret = 1;
	uint32 index = 0;
	if (get_file_name(finfo->name, path, &index) == 0)
	{
		if (get_file_name(finfo->name, path, &index) != 0)
		{
			index = 0;
			get_file_name(finfo->name, path, &index);
			if (find_from_tree(_root, NULL, finfo->name) != 0 && os_str_find(finfo->name, "/") == -1 && os_str_find(finfo->name, "\\") && -1
				&& os_str_find(finfo->name, ":") == -1 && os_str_find(finfo->name, "*") == -1 && os_str_find(finfo->name, "?") == -1 && os_str_find(finfo->name, "\"") == -1
				&& os_str_find(finfo->name, "<") == -1 && os_str_find(finfo->name, ">") == -1 && os_str_find(finfo->name, "|") == -1)
			{
				if (finfo->property | 0x00000400)
				{
					fnode *root = NULL;
					file_info *child = (file_info *)malloc(sizeof(file_info));
					file_info_init(child);
					child->property |= 0x00000600;
					os_str_cpy(child->name, ".", FS_MAX_NAME_SIZE);
					root = insert_to_btree(root, child);
					os_str_cpy(child->name, "..", FS_MAX_NAME_SIZE);
					root = insert_to_btree(root, child);
					root->finfo[0].cluster_id = root->head.node_id;
					root->finfo[1].cluster_id = _root->head.node_id;
					fnode_flush(root);
					finfo->cluster_id = root->head.node_id;
					free(child);
					free(root);
				}
				insert_to_root(finfo);
				super_cluster_flush();
				ret = 0;
			}
		}
		else
		{
			uint32 is_exist;
			fnode *node = NULL;
			index = 0;
			node = get_partent(path, &index, finfo->name, &is_exist);
			if (node != NULL)
			{
				if (!is_exist && os_str_find(finfo->name, "/") == -1 && os_str_find(finfo->name, "\\") && -1
					&& os_str_find(finfo->name, ":") == -1 && os_str_find(finfo->name, "*") == -1 && os_str_find(finfo->name, "?") == -1 && os_str_find(finfo->name, "\"") == -1
					&& os_str_find(finfo->name, "<") == -1 && os_str_find(finfo->name, ">") == -1 && os_str_find(finfo->name, "|") == -1)
				{
					if (node->finfo[index].file_count < 0xffffffff)
					{
						if (finfo->property | 0x00000400)
						{
							fnode *root = NULL;
							file_info *child = (file_info *)malloc(sizeof(file_info));
							file_info_init(child);
							child->property |= 0x00060000;
							os_str_cpy(child->name, ".", FS_MAX_NAME_SIZE);
							root = insert_to_btree(root, child);
							os_str_cpy(child->name, "..", FS_MAX_NAME_SIZE);
							root = insert_to_btree(root, child);
							root->finfo[0].cluster_id = root->head.node_id;
							root->finfo[1].cluster_id = node->finfo[index].cluster_id;
							fnode_flush(root);
							finfo->cluster_id = root->head.node_id;
							free(child);
							free(root);
						}

						insert_to_parent(&node->finfo[index], finfo);
						fnode_flush(node);
						ret = 0;
					}
				}
				if (node != _root)
				{
					free(node);
				}
				
			}
		}
	}
	return ret;
}

uint32 create_dir(const char *path)
{
	uint32 ret;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property |= 0x00000400;
	ret = do_create_file(path, finfo);
	free(finfo);
	if (0 == ret)
	{
		flush();
	}
	return ret;
}

uint32 create_file(const char *path)
{
	uint32 ret;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property &= (~0x00000400);
	ret = do_create_file(path, finfo);
	free(finfo);
	flush();
	return ret;
}

dir_ctl *open_dir(const char *path)
{
	file_info *finfo = get_file_info(path);
	if (finfo != NULL)
	{
		dir_ctl *dir = (dir_ctl *)malloc(sizeof(dir_ctl));
		dir->cur = NULL;
		dir->index = 0;
		dir->parent = NULL;
		dir->id = finfo->cluster_id;
		free(finfo);
		return dir;
	}
	return NULL;
}

void close_dir(dir_ctl *dir)
{
	if (dir->cur != NULL)
	{
		free(dir->cur);
	}
	if (dir->parent != NULL)
	{
		free(dir->parent);
	}
	free(dir);
}

uint32 read_dir(file_info *finfo, dir_ctl *dir)
{
	return query_finfo(finfo, &dir->id, &dir->parent, &dir->cur, &dir->index);
}

uint32 delete_dir()
{
	return 0;
}

uint32 delete_file()
{
	return 0;
}

uint32 open_file()
{
	return 0;
}

uint32 close_file()
{
	return 0;
}

uint32 read_file()
{
	return 0;
}

uint32 write_file()
{
	return 0;
}

uint32 seek_file()
{
	return 0;
}

void test()
{
	dir_ctl *dir;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	fs_formatting();
	fs_loading();
	create_dir("chenzhiyong");
	create_dir("/");
	create_dir("/home");
	create_dir("/home/");
	create_dir("/dev");
	create_dir("/home/chenzhiyong");
	create_dir("/home/chenzhiyong/develop");
	create_dir("/home/chenzhiyong/develop/a");
	create_dir("/home/chenzhiyong/develop/b");
	create_dir("/home/chenzhiyong/develop/c");
	create_dir("/home/chenzhiyong/develop/d");
	create_dir("/home/chenzhiyong/develop/e");
	create_dir("/home/chenzhiyong/develop/f");
	create_dir("/home/chenzhiyong/develop/g");
	create_dir("/home/chenzhiyong/develop/h");
	create_dir("/home/chenzhiyong/develop/i");
	create_dir("/home/chenzhiyong/develop/j");
	create_dir("/home/chenzhiyong/develop/k");
	create_dir("/home/chenzhiyong/develop/l");
	create_dir("/home/chenzhiyong/develop/m");
	create_dir("/home/chenzhiyong/develop/n");
	create_dir("/home/chenzhiyong/develop/o");
	create_dir("/home/chenzhiyong/develop/p");
	create_dir("/home/chenzhiyong/develop/q");
	create_dir("/home/chenzhiyong/develop/r");
	create_dir("/home/chenzhiyong/develop/s");
	create_dir("/home/chenzhiyong/develop/t");
	dir = open_dir("/home");
	while (read_dir(finfo, dir) == 0)
	{
		continue;
	}
	close_dir(dir);
	dir = open_dir("/home/chenzhiyong/develop");
	while (read_dir(finfo, dir) == 0)
	{
		continue;
	}
	free(finfo);
	close_dir(dir);
	fs_unloading();
}



