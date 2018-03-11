#include "fs/os_fs.h"
#include "lib/os_string.h"
#include "fs/os_cluster.h"
#include "fs/os_file.h"
#include <stdio.h>
#include <vtos.h>
#include <stdlib.h>
static super_cluster *_super = NULL;
static fnode *_root = NULL;
static finfo_node *_finfo_tree = NULL;

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

static char *pretreat_path(const char *path)
{
	uint32 index = 0;
	char name[FS_MAX_NAME_SIZE];
	char *buff = NULL;
	if (os_str_cmp(path, "/") == 0)
	{
		buff = (char *)malloc(FS_MAX_PATH_LEN);
		buff[0] = '/';
		buff[1] = '\0';
	}
	else if (get_file_name(name, path, &index) == 0)
	{
		uint32 *array = (uint32 *)malloc(FS_MAX_PATH_LEN * sizeof(uint32));
		int32 i;
		uint32 pre_index = 0;
		index = 0;
		array[0] = 0;
		for (i = 0; get_file_name(name, path, &index) == 0; i++)
		{
			if (os_str_cmp(name, ".") == 0)
			{
				i--;
			}
			else if (os_str_cmp(name, "..") == 0)
			{
				i--;
				if (i < 0)
				{
					break;
				}
				i--;
			}
			else
			{
				array[i] = pre_index;
			}
			pre_index = index;
		}
		if (i >= 0)
		{
			int32 j;
			uint32 k = 1;
			uint32 n;
			buff = (char *)malloc(FS_MAX_PATH_LEN);
			buff[0] = '/';
			buff[1] = '\0';
			for (j = 0; j < i; j++)
			{
				index = array[j];
				get_file_name(name, path, &index);
				if (k != 1)
				{
					buff[k] = '/';
					k++;
				}
				for (n = 0; name[n] != '\0'; n++, k++)
				{
					buff[k] = name[n];
				}
			}
			buff[k] = '\0';
		}
		free(array);
	}
	return buff;
}

static void super_cluster_init(super_cluster *super)
{
	super->flag = 0xaa55a55a;
	os_str_cpy(super->name, "emfs", FS_MAX_FSNAME_SIZE);
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
	info->property = 0x000001ff; //.10 dir or file;.9 only sys w;.876 user r,w,x;.543 group r,w,x;.210 other r,w,x
	info->file_count = 0;
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
		os_mem_cpy(temp, finfo, FS_PAGE_SIZE);
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
	finfo->property |= 0x00000600;       //设置只是系统写权限和目录标记
	os_str_cpy(finfo->name, "/", FS_MAX_NAME_SIZE);
	finfo->cluster_id = 0;
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
	if (finfo->cluster_id > 0)
	{
		_root = fnode_load(finfo->cluster_id);
	}
	else
	{
		_root = NULL;
	}
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

static fnode *get_parent(const char *path, uint32 *idx, char *child_name, uint32 *is_exist)
{
	uint32 index = 0;
	fnode *ret = NULL;
	uint32 flag = 0;
	*is_exist = 0;
	*idx = FS_MAX_KEY_NUM;
	if (get_file_name(child_name, path, &index) == 0)
	{
		char parent_name[FS_MAX_NAME_SIZE];
		fnode *cur = _root;
		ret = _root;
		os_str_cpy(parent_name, child_name, FS_MAX_NAME_SIZE);
		while (get_file_name(child_name, path, &index) == 0)
		{
			if (ret != _root)
			{
				free(ret);
			}
			ret = find_from_tree2(cur, idx, parent_name);
			if (ret != NULL && ret->finfo[*idx].cluster_id > 0 && ret->finfo[*idx].property & 0x00000400)
			{
				if (cur != ret)
				{
					free(cur);
				}
				cur = fnode_load(ret->finfo[*idx].cluster_id);
			}
			else
			{
				flag = 1;
				break;
			}
			os_str_cpy(parent_name, child_name, FS_MAX_NAME_SIZE);
		}
		if (0 == flag && find_from_tree(cur, NULL, child_name) == 0)
		{
			*is_exist = 1;
		}

		if (cur != NULL && cur != _root && cur != ret)
		{
			free(cur);
		}

		if (flag && get_file_name(child_name, path, &index) != 0)
		{
			flag = 0;
		}

		if (flag)
		{
			if (ret != NULL && ret != _root)
			{
				free(ret);
				ret = NULL;
			}
		}
	}
	return ret;
}

static file_info *get_file_info(const char *path)
{
	uint32 flag = 1;
	uint32 index = 0;
	char name[FS_MAX_NAME_SIZE];
	fnode *cur = _root;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	while (get_file_name(name, path, &index) == 0)
	{
		if (find_from_tree(cur, finfo, name) == 0)
		{
			flag = 0;
			if (finfo->cluster_id > 0 && finfo->property & 0x00000400)  //判断是否为目录
			{
				if (cur != _root)
				{
					free(cur);
				}
				cur = fnode_load(finfo->cluster_id);
			}
			else
			{
				break;
			}
		}
		else
		{
			flag = 1;
			break;
		}
	}
	if (get_file_name(name, path, &index) == 0)
	{
		flag = 1;
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

static dir_obj *init_dir_obj(uint32 id)
{
	dir_obj *dir = (dir_obj *)malloc(sizeof(dir_obj));
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

static void remove_from_root(const char *name)
{
	fnode *root = remove_from_btree(_root, name);
	if (root != _root)
	{
		file_info *finfo = file_info_load(_super->root_id);
		_root = root;
		if (_root != NULL)
		{
			finfo->cluster_id = _root->head.node_id;
		}
		else
		{
			finfo->cluster_id = 0;
		}
		file_info_flush(_super->root_id, finfo);
		free(finfo);
	}

}

static void insert_to_parent(file_info *parent, file_info *child)
{
	fnode *root = NULL;
	if (parent->cluster_id > 0)
	{
		root = fnode_load(parent->cluster_id);
	}
	root = insert_to_btree(root, child);
	parent->file_count++;
	parent->cluster_id = root->head.node_id;
	free(root);
}

static fnode *remove_from_parent(file_info *parent_info, fnode *parent, const char *name)
{
	fnode *new_parent = remove_from_btree(parent, name);
	if (new_parent != parent)
	{
		if (new_parent != NULL)
		{
			parent_info->file_count--;
			parent_info->cluster_id = new_parent->head.node_id;
		}
		else
		{
			parent_info->file_count--;
			parent_info->cluster_id = 0;
		}
	}
	return new_parent;
}

static uint32 do_create_file(const char *path, file_info *finfo)
{
	uint32 ret = 1;
	uint32 flag = 0;
	uint32 index = 0;
	fnode *node = NULL;
	fnode *temp = _root;
	if (NULL == _root)
	{
		if (get_file_name(finfo->name, path, &index) == 0 && get_file_name(finfo->name, path, &index) != 0)
		{
			flag = 1;
		}
	}
	else
	{
		uint32 is_exist;
		node = get_parent(path, &index, finfo->name, &is_exist);
		if (node != NULL)
		{
			if (FS_MAX_KEY_NUM == index && find_from_tree(_root, NULL, finfo->name) != 0)                //父母是根目录
			{
				flag = 1;
			}
			else if (index < FS_MAX_KEY_NUM && !is_exist && node->finfo[index].property & 0x00000400 && node->finfo[index].file_count < 0xffffffff)
			{
				flag = 2;
			}
		}
	}
	if (flag > 0 && os_str_find(finfo->name, "/") == -1 && os_str_find(finfo->name, "\\") && os_str_find(finfo->name, ".") == -1 && os_str_find(finfo->name, "..")
		&& os_str_find(finfo->name, ":") == -1 && os_str_find(finfo->name, "*") == -1 && os_str_find(finfo->name, "?") == -1 && os_str_find(finfo->name, "\"") == -1
		&& os_str_find(finfo->name, "<") == -1 && os_str_find(finfo->name, ">") == -1 && os_str_find(finfo->name, "|") == -1)
	{
		if (1 == flag)
		{
			insert_to_root(finfo);
		}
		else
		{
			insert_to_parent(&node->finfo[index], finfo);
			fnode_flush(node);
		}

		ret = 0;
	}

	if (node != NULL && node != temp)
	{
		free(node);
	}
	return ret;
}

uint32 create_dir(const char *path)
{
	uint32 ret = 1;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		file_info *finfo = (file_info *)malloc(sizeof(file_info));
		file_info_init(finfo);
		finfo->property |= 0x00000400;  //设置目录标记
		ret = do_create_file(pre_path, finfo);
		free(finfo);
		if (0 == ret)
		{
			flush();
		}
		free(pre_path);
	}
	return ret;
}

uint32 create_file(const char *path)
{
	uint32 ret = 1;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		file_info *finfo = (file_info *)malloc(sizeof(file_info));
		file_info_init(finfo);
		finfo->property &= (~0x00000400);  //设置文件标记
		ret = do_create_file(pre_path, finfo);
		free(finfo);
		if (0 == ret)
		{
			flush();
		}
		free(pre_path);
	}
	return ret;
}

dir_obj *open_dir(const char *path)
{
	uint32 id = 0;
	uint32 flag = 0;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		if (os_str_cmp(pre_path, "/") == 0)
		{
			flag = 1;
			if (_root != NULL)
			{
				id = _root->head.node_id;
			}
		}
		else
		{
			file_info *finfo = get_file_info(pre_path);
			if (finfo != NULL && (finfo->property & 0x00000400))
			{
				flag = 1;
				id = finfo->cluster_id;
				free(finfo);
			}
		}
		if (flag)
		{
			dir_obj *dir = (dir_obj *)malloc(sizeof(dir_obj));
			dir->cur = NULL;
			dir->index = 0;
			dir->parent = NULL;
			dir->id = id;
			free(pre_path);
			return dir;
		}
		free(pre_path);
	}
	return NULL;
}

void close_dir(dir_obj *dir)
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

uint32 read_dir(file_info *finfo, dir_obj *dir)
{
	if (dir->id > 0)
	{
		return query_finfo(finfo, &dir->id, &dir->parent, &dir->cur, &dir->index);
	}
	return 1;
}

uint32 delete_dir(const char *path)
{
	uint32 ret = 1;
	if (_root != NULL)
	{
		char *pre_path = pretreat_path(path);
		if (pre_path != NULL)
		{
			uint32 index = 0;
			char name[FS_MAX_NAME_SIZE];
			uint32 is_exist = 0;
			fnode *node = get_parent(pre_path, &index, name, &is_exist);
			if (node != NULL)
			{
				fnode *temp = _root;
				if (FS_MAX_KEY_NUM == index)                //父母是根目录
				{
					file_info *finfo = (file_info *)malloc(sizeof(file_info));
					if (find_from_tree(_root, finfo, name) == 0)
					{
						if (0 == finfo->file_count && finfo->property & 0x00000400 && (finfo->property & 0x00000200) == 0)
						{
							remove_from_root(name);
							ret = 0;
						}
					}
					free(finfo);
				}
				else if (index < FS_MAX_KEY_NUM && is_exist)
				{
					fnode *parent = NULL;
					file_info *finfo = (file_info *)malloc(sizeof(file_info));
					parent = fnode_load(node->finfo[index].cluster_id);
					if (find_from_tree(parent, finfo, name) == 0)
					{
						if (0 == finfo->file_count && finfo->property & 0x00000400 && (finfo->property & 0x00000200) == 0)
						{
							parent = remove_from_parent(&node->finfo[index], parent, name);
							fnode_flush(node);
							ret = 0;
						}
					}
					free(parent);
					free(finfo);
				}
				if (node != NULL && node != temp)
				{
					free(node);
				}
				if (0 == ret)
				{
					flush();
				}
			}
			free(pre_path);
		}
	}
	return ret;
}

uint32 delete_file(const char *path)
{
	uint32 ret = 1;
	if (_root != NULL)
	{
		char *pre_path = pretreat_path(path);
		if (pre_path != NULL)
		{
			uint32 index = 0;
			char name[FS_MAX_NAME_SIZE];
			uint32 is_exist = 0;
			fnode *node = get_parent(pre_path, &index, name, &is_exist);
			if (node != NULL)
			{
				fnode *temp = _root;
				if (FS_MAX_KEY_NUM == index)                //父母是根目录
				{
					file_info *finfo = (file_info *)malloc(sizeof(file_info));
					if (find_from_tree(_root, finfo, name) == 0)
					{
						if ((finfo->property & 0x00000400) == 0 && (finfo->property & 0x00000200) == 0)
						{
							if (finfo->cluster_id > 0)
							{
								file_data_remove(finfo->cluster_id, finfo->cluster_count);
							}
							remove_from_root(name);
							ret = 0;
						}
					}
					free(finfo);
				}
				else if (index < FS_MAX_KEY_NUM && is_exist)
				{
					fnode *parent = NULL;
					file_info *finfo = (file_info *)malloc(sizeof(file_info));
					parent = fnode_load(node->finfo[index].cluster_id);
					if (find_from_tree(parent, finfo, name) == 0)
					{
						if ((finfo->property & 0x00000400) == 0 && (finfo->property & 0x00000200) == 0)
						{
							if (finfo->cluster_id > 0)
							{
								file_data_remove(finfo->cluster_id, finfo->cluster_count);
							}
							parent = remove_from_parent(&node->finfo[index], parent, name);
							fnode_flush(node);
							ret = 0;
						}
					}
					free(parent);
					free(finfo);
				}
				if (node != NULL && node != temp)
				{
					free(node);
				}
				if (0 == ret)
				{
					flush();
				}
			}
			free(pre_path);
		}
	}
	return ret;
}

static uint32 insert_empty_file(const char *dir)
{
	uint32 ret = 1;
	uint32 index = 0;
	char name[FS_MAX_NAME_SIZE];
	uint32 is_exist = 0;
	fnode *node = get_parent(dir, &index, name, &is_exist);
	if (node != NULL)
	{
		fnode *temp = _root;
		if (FS_MAX_KEY_NUM == index)                //父母是根目录
		{
			if (find_from_tree(_root, NULL, name) != 0)
			{
				file_info *finfo = (file_info *)malloc(sizeof(file_info));
				os_str_cpy(finfo->name, name, FS_MAX_NAME_SIZE);
				insert_to_root(finfo);
				free(finfo);
				ret = 0;
			}
		}
		else if (index < FS_MAX_KEY_NUM && node->finfo[index].file_count < 0xffffffff && !is_exist)
		{
			file_info *finfo = (file_info *)malloc(sizeof(file_info));
			os_str_cpy(finfo->name, name, FS_MAX_NAME_SIZE);
			insert_to_parent(&node->finfo[index], finfo);
			fnode_flush(node);
			free(finfo);
			ret = 0;
		}
		if (node != NULL && node != temp)
		{
			free(node);
		}
	}
	return ret;
}

uint32 move_file(const char *dest, const char *src)
{
	uint32 ret = 1;
	char *pre_src = pretreat_path(src);
	if (pre_src != NULL)
	{
		char *pre_dest = pretreat_path(dest);
		if (pre_dest != NULL)
		{
			if (insert_empty_file(pre_dest) == 0)
			{
				uint32 index = 0;
				char name[FS_MAX_NAME_SIZE];
				uint32 is_exist = 0;
				fnode *node = get_parent(pre_src, &index, name, &is_exist);
				if (node != NULL)
				{
					fnode *temp = _root;
					file_info *finfo = (file_info *)malloc(sizeof(file_info));
					if (FS_MAX_KEY_NUM == index)                //父母是根目录
					{
						if (find_from_tree(_root, finfo, name) == 0)
						{
							remove_from_root(name);
							ret = 0;
						}
					}
					else if (index < FS_MAX_KEY_NUM && is_exist)
					{
						fnode *parent = NULL;
						parent = fnode_load(node->finfo[index].cluster_id);
						if (find_from_tree(parent, finfo, name) == 0)
						{
							parent = remove_from_parent(&node->finfo[index], parent, name);
							fnode_flush(node);
							ret = 0;
						}
						free(parent);
					}
					if (node != NULL && node != temp)
					{
						free(node);
					}

					if (0 == ret)
					{
						ret = 1;
						node = get_parent(pre_dest, &index, name, &is_exist);
						if (node != NULL)
						{
							file_info *new_finfo = (file_info *)malloc(sizeof(file_info));
							temp = _root;
							if (FS_MAX_KEY_NUM == index)                //父母是根目录
							{
								if (find_from_tree(_root, new_finfo, name) == 0)
								{
									os_str_cpy(finfo->name, new_finfo->name, sizeof(FS_MAX_NAME_SIZE));
									os_mem_cpy(new_finfo, finfo, sizeof(file_info));
									remove_from_root(new_finfo->name);
									insert_to_root(new_finfo);
									ret = 0;
								}
							}
							else if (index < FS_MAX_KEY_NUM && is_exist)
							{
								fnode *parent = NULL;
								parent = fnode_load(node->finfo[index].cluster_id);
								if (find_from_tree(parent, new_finfo, name) == 0)
								{
									os_str_cpy(finfo->name, new_finfo->name, sizeof(FS_MAX_NAME_SIZE));
									os_mem_cpy(new_finfo, finfo, sizeof(file_info));
									parent = remove_from_parent(&node->finfo[index], parent, name);
									insert_to_parent(&node->finfo[index], new_finfo);
									fnode_flush(node);
									ret = 0;
								}
								free(parent);
							}
							if (node != NULL && node != temp)
							{
								free(node);
							}

							if (0 == ret)
							{
								flush();
							}
							free(new_finfo);
						}
					}
					free(finfo);
				}
			}
			free(pre_dest);
		}
		free(pre_src);
	}
	return ret;
}

static void insert_to_finfo_tree(tree_node_type_def **handle, finfo_node *node)
{
	finfo_node *cur_node = (finfo_node *)*handle;
	os_init_node(&(node->tree_node_structrue));
	if (NULL == *handle)
	{
		node->tree_node_structrue.color = BLACK;
		*handle = &(node->tree_node_structrue);
	}
	else
	{
		for (;;)
		{
			if (node->key <= cur_node->key)
			{
				if (cur_node->tree_node_structrue.left_tree == &_leaf_node)
				{
					break;
				}
				cur_node = (finfo_node *)cur_node->tree_node_structrue.left_tree;
			}
			else
			{
				if (cur_node->tree_node_structrue.right_tree == &_leaf_node)
				{
					break;
				}
				cur_node = (finfo_node *)cur_node->tree_node_structrue.right_tree;
			}
		}
		node->tree_node_structrue.parent = &cur_node->tree_node_structrue;
		if (node->key <= cur_node->key)
			cur_node->tree_node_structrue.left_tree = &(node->tree_node_structrue);
		else
			cur_node->tree_node_structrue.right_tree = &(node->tree_node_structrue);
		os_insert_case(handle, &(node->tree_node_structrue));
	}
}

static void remove_from_finfo_tree(tree_node_type_def **handle, finfo_node *node)
{
	os_delete_node(handle, &(node->tree_node_structrue));
}

static finfo_node *find_finfo_node(finfo_node *finfo_tree, uint32 key)
{
	finfo_node *cur = finfo_tree;
	if (cur != NULL)
	{
		for (;;)
		{
			if (cur->key == key)
			{
				return cur;
			}
			else if (key < cur->key)
			{
				if (cur->tree_node_structrue.left_tree == &_leaf_node)
				{
					break;
				}
				cur = (finfo_node *)cur->tree_node_structrue.left_tree;
			}
			else
			{
				if (cur->tree_node_structrue.right_tree == &_leaf_node)
				{
					break;
				}
				cur = (finfo_node *)cur->tree_node_structrue.right_tree;
			}
		}
	}
	return NULL;
}

file_obj *open_file(const char *path, uint32 flags)
{
	char *pre_path = pretreat_path(path);
	if(pre_path != NULL)
	{
		file_info *finfo = NULL;
		if (flags & FS_CREATE)
		{
			delete_file(pre_path);
			create_file(pre_path);
		}
		finfo = get_file_info(pre_path);
		if (finfo != NULL && (finfo->property & 0x00000400) == 0)  //检查文件属性
		{
			file_obj *file = NULL;
			finfo_node *node = find_finfo_node(_finfo_tree, finfo->cluster_id);
			if (node != NULL)
			{
				if (node->count < 0xffffffff)
				{
					node->count++;
				}
				else
				{
					free(finfo);
					return NULL;
				}
			}
			else
			{
				node = (finfo_node *)malloc(sizeof(finfo_node));
				node->count = 1;
				node->key = finfo->cluster_id;
				os_str_cpy(node->path, pre_path, FS_MAX_PATH_LEN);
				os_mem_cpy(&node->finfo, finfo, sizeof(file_info));
				insert_to_finfo_tree((tree_node_type_def **)(&_finfo_tree), node);
			}
			file = (file_obj *)malloc(sizeof(file_obj));
			file->flags = flags;
			if (flags & FS_APPEND)
			{
				file->index = node->finfo.size - 1;
			}
			else
			{
				file->index = 0;
			}
			file->node = node;
			free(finfo);
			return file;
		}
		free(pre_path);
	}
	return NULL;
}

void close_file(file_obj *file)
{
	finfo_node *node = find_finfo_node(_finfo_tree, file->node->finfo.cluster_id);
	node->count--;
	if (0 == node->count)
	{
		remove_from_finfo_tree((tree_node_type_def **)(&_finfo_tree), file->node);
	}
	{
		uint32 is_exist;
		uint32 index = 0;
		fnode *n = get_parent(file->node->path, &index, file->node->finfo.name, &is_exist);
		if (n != NULL && is_exist)
		{
			os_mem_cpy(&n->finfo[index], &file->node->finfo, sizeof(file_info));
			fnode_flush(n);
			if (_root != n)
			{
				free(n);
			}
		}
	}
	free(file);
}

uint32 read_file(file_obj *file, void *data, uint32 len)
{
	if ((file->flags & FS_READ) && (file->node->finfo.property & 0x00000124)) //判断是否具有读权限
	{
		if (file->index + len > file->node->finfo.size)
		{
			len = (uint32)(file->node->finfo.size - file->index);
		}
		if (len > 0)
		{
			if (1 == file->node->finfo.cluster_count)  //按照小文件方式读取
			{
				little_file_data_read(file->node->finfo.cluster_id, file->index, data, len);
			}
			else
			{
				file_data_read(file->node->finfo.cluster_id, file->node->finfo.cluster_count, file->index, data, len);
				file->index += len;
			}
			return len;
		}
	}
	return 0;
}

uint32 write_file(file_obj *file, void *data, uint32 len)
{
	if ((file->flags & FS_WRITE) && (file->node->finfo.property & 0x00000092)) //判断是否具有写权限
	{
		if (len > 0)
		{
			if (file->index + len <= FS_PAGE_SIZE && 1 == file->node->finfo.cluster_count)  //按照小文件方式写入
			{
				little_file_data_write(file->node->finfo.cluster_id, file->index, data, len);
			}
			else
			{
				file->node->finfo.cluster_id = file_data_write(file->node->finfo.cluster_id, &file->node->finfo.cluster_count, file->index, data, len);
			}

			if (file->index + len > file->node->finfo.size)
			{
				file->node->finfo.size = file->index + len;
			}
			file->index += len;
			file->node->finfo.modif_time = os_get_time();
			return len;
		}
	}
	return 0;
}

uint32 seek_file(file_obj *file, int64 offset, uint32 fromwhere)
{
	uint64 temp;
	switch (fromwhere)
	{
	case FS_SEEK_SET:
	{
		temp = offset;
		break;
	}
	case FS_SEEK_CUR:
	{
		temp = file->index + offset;
		break;
	}
	case FS_SEEK_END:
	{
		temp = file->node->finfo.size + offset;
		break;
	}
	}
	if (temp <= file->node->finfo.size)
	{
		file->index = temp;
		return 0;
	}
	return 1;
}



