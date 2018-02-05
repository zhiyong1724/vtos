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
		ret = 0;
		(*index)++;
		for (; path[*index] != '\0'; (*index)++)
		{
			if (path[*index] != '/')
			{
				break;
			}
		}
		for (; path[*index] != '\0'; (*index)++)
		{
			if (path[*index] == '/')
			{
				break;
			}
			else
			{
				*dest = path[*index];
				dest++;
			}
		}
		*dest = '\0';
	}
	return ret;
}

static void super_cluster_init(super_cluster *super)
{
	super->flag = 0xaa55a55a;
	super->cluster_id = 0;
	os_str_cpy(super->name, "emfs", 16);
	super->bitmap_id = 0;
	super->root_id = 0;
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
	info->property = 0x000001ff;
	info->file_count = 0;
	info->name[0] = '\0';
}

static uint32 super_cluster_flush()
{
	uint32 ret = CLUSTER_NONE;
	if (is_little_endian())
	{
		ret = cluster_write(_super->cluster_id, (uint8 *)_super);
		if (CLUSTER_NONE == ret)
		{
			ret = cluster_write(_super->cluster_id + 1, (uint8 *)_super);
		}
	}
	else
	{
		super_cluster *temp = (super_cluster *)malloc(FS_PAGE_SIZE);
		os_mem_cpy(temp, _super, FS_PAGE_SIZE);

		temp->flag = convert_endian(_super->flag);
		temp->cluster_id = convert_endian(_super->cluster_id);
		temp->bitmap_id = convert_endian(_super->bitmap_id);
		temp->root_id = convert_endian(_super->root_id);
		temp->backup_id = convert_endian(_super->backup_id);
		ret = cluster_write(_super->cluster_id, (uint8 *)temp);
		if (CLUSTER_NONE == ret)
		{
			ret = cluster_write(_super->cluster_id + 1, (uint8 *)temp);
		}
		free(temp);
	}
	return ret;
}

static uint32 handle_error()
{
	uint32 ret = 1;
	for (_super->cluster_id += 2; _super->cluster_id < RETAIN_AREA_SIZE - 1; _super->cluster_id += 2)
	{
		if (super_cluster_flush() == CLUSTER_NONE)
		{
			ret = 0;
			break;
		}
	}
	return ret;
}

static uint32 super_cluster_load2(uint32 cluster)
{
	uint32 ret = 1;
	if (is_little_endian())
	{
		ret = cluster_read(cluster, (uint8 *)_super);
		if (ret != CLUSTER_NONE)
		{
			ret = cluster_read(cluster + 1, (uint8 *)_super);
		}
	}
	else
	{
		super_cluster *temp = (super_cluster *)malloc(FS_PAGE_SIZE);
		ret = cluster_read(cluster, (uint8 *)temp);
		if (ret != CLUSTER_NONE)
		{
			ret = cluster_read(cluster + 1, (uint8 *)temp);
		}
		if (CLUSTER_NONE == ret)
		{
			os_mem_cpy(_super, temp, FS_PAGE_SIZE);
			_super->flag = convert_endian(temp->flag);
			_super->cluster_id = convert_endian(temp->cluster_id);
			_super->bitmap_id = convert_endian(temp->bitmap_id);
			_super->root_id = convert_endian(temp->root_id);
			_super->backup_id = convert_endian(temp->backup_id);
		}
		free(temp);
	}
	return ret;
}

static uint32 super_cluster_load(uint32 cluster)
{
	uint32 ret = 1;
	if (is_little_endian())
	{
		if (cluster_read(cluster, (uint8 *)_super) == CLUSTER_NONE)
		{
			if (cluster == _super->cluster_id)
			{
				if (cluster_read(cluster + 1, (uint8 *)_super) == CLUSTER_NONE)
				{
					if (cluster == _super->cluster_id)
					{
						ret = 0;
					}
				}
			}
		}
	}
	else
	{
		if (cluster_read(cluster, (uint8 *)_super) == CLUSTER_NONE)
		{
			_super->flag = convert_endian(_super->flag);
			_super->cluster_id = convert_endian(_super->cluster_id);
			_super->bitmap_id = convert_endian(_super->bitmap_id);
			_super->root_id = convert_endian(_super->root_id);
			_super->backup_id = convert_endian(_super->backup_id);
			if (cluster == _super->cluster_id)
			{
				if (cluster_read(cluster + 1, (uint8 *)_super) == CLUSTER_NONE)
				{
					_super->flag = convert_endian(_super->flag);
					_super->cluster_id = convert_endian(_super->cluster_id);
					_super->bitmap_id = convert_endian(_super->bitmap_id);
					_super->root_id = convert_endian(_super->root_id);
					_super->backup_id = convert_endian(_super->backup_id);
					if (cluster == _super->cluster_id)
					{
						ret = 0;
					}
				}
			}
		}
	}
	return ret;
}

static uint32 super_cluster_flush2()
{
	uint32 ret = super_cluster_flush();
	if (ret != CLUSTER_NONE)
	{
		ret = handle_error();
	}
	return ret;
}

static void on_cluster_manager_change(uint32 cluster_id)
{
	_super->bitmap_id = cluster_id;
	super_cluster_flush2(_super->cluster_id);
}

uint32 fs_formatting()
{
	uint32 ret = 1;
	while(sizeof(fnode) != FS_PAGE_SIZE);
	_super = (super_cluster *)malloc(FS_PAGE_SIZE);
	cluster_controler_init();
	register_manager_callback(on_cluster_manager_change);
	super_cluster_init(_super);
	_super->bitmap_id = cluster_manager_init();
	if (_super->bitmap_id > 0)
	{
		for (_super->cluster_id = 1; _super->cluster_id < RETAIN_AREA_SIZE - 1; _super->cluster_id += 2)
		{
			if (super_cluster_flush() == CLUSTER_NONE)
			{
				break;
			}
		}
		if (_super->cluster_id < RETAIN_AREA_SIZE - 1)
		{
			ret = 0;
		}
	}
	fs_unloading();
	return ret;
}

uint32 fs_loading()
{
	uint32 ret = 1;
	uint32 i;
	while(sizeof(fnode) != FS_PAGE_SIZE);
	_super = (super_cluster *)malloc(FS_PAGE_SIZE);
	cluster_controler_init();
	register_manager_callback(on_cluster_manager_change);
	for (i = 1; i < RETAIN_AREA_SIZE - 1; i += 2)
	{
		if (super_cluster_load(i) == CLUSTER_NONE && _super->flag == 0xaa55a55a)
		{
			ret = 0;
			break;
		}
	}
	if (ret != 0)
	{
		for (i = 1; i < RETAIN_AREA_SIZE - 1; i += 2)
		{
			if (super_cluster_load2(i) == CLUSTER_NONE && _super->flag == 0xaa55a55a)
			{
				ret = 0;
				break;
			}
		}
	}
	if (_super->root_id != 0 && 0 == ret)
	{
		ret = 1;
		_root = fnode_load(_super->root_id);
		if (NULL == _root && _super->backup_id != 0)
		{
			_root = fnode_load(_super->backup_id);
		}
		if (_root != NULL)
		{
			ret = 0;
		}
	}
	if (0 == ret)
	{
		ret = cluster_manager_load(_super->bitmap_id);
	}
	if (ret != 0)
	{
		free(_super);
	}
	return ret;
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

static fnode *get_partent(const char *path, uint32 *idx, char *child_name)
{
	uint32 index = 0;
	char parent_name[FS_MAX_NAME_SIZE];
	fnode *cur = _root;
	fnode *ret = _root;
	if (get_file_name(child_name, path, &index) == 0)
	{
		os_str_cpy(parent_name, child_name, FS_MAX_NAME_SIZE);
		while (get_file_name(child_name, path, &index) == 0)
		{
			if (ret != _root)
			{
				free(ret);
			}
			ret = find_from_tree2(cur, idx, parent_name);
			if (ret != NULL)
			{
				if (ret->finfo[*idx].file_count > 0)
				{
					if (cur != _root)
					{
						free(cur);
					}
					cur = fnode_load(ret->finfo[*idx].cluster_id);
				}
			}
			else
			{
				break;
			}
		}
		if (cur != NULL && cur != _root)
		{
			free(cur);
		}
	}
	return ret;
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

static uint32 insert_to_super(file_info *child)
{
	uint32 ret = 1;
	uint32 status1 = 1;
	uint32 status2 = 1;
	fnode *backup = fnode_load(_super->backup_id);
	fnode *root1 = insert_to_btree(_root, child, &status1);
	if (0 == status1)
	{
		if (_root != root1)
		{
			ret = 0;
			_root = root1;
			_super->root_id = root1->head.node_id;
		}
	}
	if (backup != NULL)
	{
		fnode *backup1 = insert_to_btree(backup, child, &status2);
		if (0 == status2)
		{
			if (backup != backup1)
			{
				ret = 0;
				backup = backup1;
				_super->backup_id = backup1->head.node_id;
			}
		}
	}

	if (0 == status1 && status2 != 0)
	{
		dir_ctl *dir = init_dir_ctl(_super->root_id);
		fnode *backup1 = NULL;
		file_info *finfo = NULL;
		while((finfo = read_dir(dir)) != NULL)
		{
			backup1 = insert_to_btree(backup1, finfo, &status2);
			free(finfo);
		}
		close_dir(dir);
		if (backup1 != NULL)
		{
			ret = 0;
			_super->backup_id = backup1->head.node_id;
			free(backup1);
		}
	}

	if (status1 != 0 && 0 == status2)
	{
		dir_ctl *dir = init_dir_ctl(_super->backup_id);
		fnode *root1 = NULL;
		file_info *finfo = NULL;
		while((finfo = read_dir(dir)) != NULL)
		{
			root1 = insert_to_btree(root1, finfo, &status1);
			free(finfo);
		}
		close_dir(dir);
		if (root1 != NULL)
		{
			ret = 0;
			_super->root_id = root1->head.node_id;
			free(_root);
			_root = root1;
		}
	}
	if (backup != NULL)
	{
		free(backup);
	}
	return ret;
}

static uint32 remove_from_super(file_info *child)
{
	uint32 ret = 1;
	uint32 status1 = 1;
	uint32 status2 = 1;
	fnode *backup = fnode_load(_super->backup_id);
	fnode *root1 = remove_from_btree(_root, child->name, &status1);
	if (0 == status1)
	{
		if (NULL == root1)
		{
			ret = 0;
			_root = root1;
			_super->root_id = 0;
		}
		else if (_root != root1)
		{
			ret = 0;
			_root = root1;
			_super->root_id = root1->head.node_id;
		}
	}
	if (backup != NULL)
	{
		fnode *backup1 = remove_from_btree(backup, child->name, &status2);
		if (0 == status2)
		{
			if (NULL == backup1)
			{
				ret = 0;
				backup = backup1;
				_super->backup_id = 0;
			}
			else if (backup != backup1)
			{
				ret = 0;
				backup = backup1;
				_super->backup_id = backup1->head.node_id;
			}
		}
	}

	if (0 == status1 && status2 != 0)
	{
		if (_root != NULL)
		{
			dir_ctl *dir = init_dir_ctl(_super->root_id);
			fnode *backup1 = NULL;
			file_info *finfo = NULL;
			while((finfo = read_dir(dir)) != NULL)
			{
				backup1 = insert_to_btree(backup1, finfo, &status2);
				free(finfo);
			}
			close_dir(dir);
			if (backup1 != NULL)
			{
				ret = 0;
				_super->backup_id = backup1->head.node_id;
				free(backup1);
			}
		}
		else
		{
			ret = 0;
			_super->backup_id = 0;
		}
		
	}

	if (status1 != 0 && 0 == status2)
	{
		if (_super->backup_id != 0)
		{
			dir_ctl *dir = init_dir_ctl(_super->backup_id);
			fnode *root1 = NULL;
			file_info *finfo = NULL;
			while((finfo = read_dir(dir)) != NULL)
			{
				root1 = insert_to_btree(root1, finfo, &status1);
				free(finfo);
			}
			close_dir(dir);
			if (root1 != NULL)
			{
				ret = 0;
				_super->root_id = root1->head.node_id;
				free(_root);
				_root = root1;
			}
		}
		else
		{
			ret = 0;
			_super->root_id = 0;
		}
	}
	if (backup != NULL)
	{
		free(backup);
	}
	
	return ret;
}

static uint32 insert_to_parent(file_info *parent, file_info *child)
{
	uint32 ret = 1;
	uint32 status;
	fnode *root = NULL;
	if (parent->cluster_id != 0)
	{
		root = fnode_load(parent->cluster_id);
		if (root != NULL)
		{
			root = insert_to_btree(root, child, &status);
			if (0 == status)
			{
				parent->file_count++;
				ret = 0;
				parent->cluster_id = root->head.node_id;
			}
			free(root);
		}
	}
	else
	{
		root = insert_to_btree(root, child, &status);
		if (0 == status)
		{
			parent->file_count++;
			ret = 0;
			parent->cluster_id = root->head.node_id;
		}
	}
	return ret;
}

static uint32 remove_from_parent(file_info *parent, file_info *child)
{
	uint32 ret = 1;
	uint32 status;
	fnode *root = NULL;
	if (parent->cluster_id != 0)
	{
		root = fnode_load(parent->cluster_id);
		if (root != NULL)
		{
			root = remove_from_btree(root, child->name, &status);
			if (0 == status)
			{
				if (root != NULL)
				{
					parent->file_count--;
					parent->cluster_id = root->head.node_id;
				}
				else
				{
					parent->file_count = 0;
					parent->cluster_id = 0;
				} 
				ret = 0;
			}
			if (root != NULL)
			{
				free(root);
			}
		}
	}
	return ret;
}

static uint32 do_create_file(const char *path, file_info *finfo)
{
	uint32 ret = 1;
	uint32 index = 0;
	if (_root != NULL)
	{
		if (get_file_name(finfo->name, path, &index) == 0 && get_file_name(finfo->name, path, &index) != 0)
		{
			index = 0;
			get_file_name(finfo->name, path, &index);
			if (finfo->name[0] != '\0' && os_str_find(finfo->name, "/") == -1 && os_str_find(finfo->name, "\\") && -1 && os_str_find(finfo->name, ":") == -1
				&& os_str_find(finfo->name, "*") == -1 && os_str_find(finfo->name, "?") == -1 && os_str_find(finfo->name, "\"") == -1
				&& os_str_find(finfo->name, "<") == -1 && os_str_find(finfo->name, ">") == -1 && os_str_find(finfo->name, "|") == -1)
			{
				if (insert_to_super(finfo) == 0)
				{
					super_cluster_flush2();
				}
				ret = 0;
			}
		}
		else
		{
			fnode *node = NULL;
			index = 0;
			node = get_partent(path, &index, finfo->name);
			if (node != NULL)
			{
				if (finfo->name[0] != '\0' && os_str_find(finfo->name, "/") == -1 && os_str_find(finfo->name, "\\") && -1 && os_str_find(finfo->name, ":") == -1
					&& os_str_find(finfo->name, "*") == -1 && os_str_find(finfo->name, "?") == -1 && os_str_find(finfo->name, "\"") == -1
					&& os_str_find(finfo->name, "<") == -1 && os_str_find(finfo->name, ">") == -1 && os_str_find(finfo->name, "|") == -1)
				{
					if (node->finfo[index].file_count < 0xffffffff)
					{
						if (insert_to_parent(&node->finfo[index], finfo) == 0)
						{
							fnode_flush(node);
							ret = 0;
						}
					}
				}

			}
		}
	}
	else
	{
		if (get_file_name(finfo->name, path, &index) == 0 && get_file_name(finfo->name, path, &index) != 0)
		{
			if (finfo->name[0] != '\0' && os_str_find(finfo->name, "/") == -1 && os_str_find(finfo->name, "\\") && -1 && os_str_find(finfo->name, ":") == -1
				&& os_str_find(finfo->name, "*") == -1 && os_str_find(finfo->name, "?") == -1 && os_str_find(finfo->name, "\"") == -1
				&& os_str_find(finfo->name, "<") == -1 && os_str_find(finfo->name, ">") == -1 && os_str_find(finfo->name, "|") == -1)
			{
				uint32 status;
				index = 0;
				_root = insert_to_btree(_root, finfo, &status);
				if (0 == status)
				{
					_super->backup_id = _root->head.node_id;
					free(_root);
					_root = NULL;
					_root = insert_to_btree(_root, finfo, &status);
					if (0 == status)
					{
						_super->root_id = _root->head.node_id;
						ret = 0;
						super_cluster_flush2();
					}
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
	finfo->property |= 0x000020;
	ret = do_create_file(path, finfo);
	free(finfo);
	if (0 == ret)
	{
		flush2();
	}
	return ret;
}

uint32 create_file(const char *path)
{
	uint32 ret;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property &= (~0x000020);
	ret = do_create_file(path, finfo);
	free(finfo);
	flush2();
	return ret;
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

dir_ctl *open_dir()
{
	dir_ctl *dir = (dir_ctl *)malloc(sizeof(dir_ctl));
	dir->cur = NULL;
	dir->id = 0;
	dir->index = 0;
	dir->parent = NULL;
	return dir;
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

file_info *read_dir(dir_ctl *dir)
{
	return query_finfo(&dir->id, &dir->parent, &dir->cur, &dir->index);
}

uint32 find_file()
{
	return 0;
}

void test()
{
	fs_formatting();
	fs_loading();
	create_dir("chenzhiyong");
	create_dir("/");
	create_dir("/home");
	create_dir("/dev");
	create_dir("/home/chenzhiyong");
	fs_unloading();
}



