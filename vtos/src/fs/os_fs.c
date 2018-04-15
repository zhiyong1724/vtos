#include "fs/os_fs.h"
#include "base/os_string.h"
#include "fs/os_cluster.h"
#include "fs/os_file.h"
#include "fs/os_journal.h"
#include <stdio.h>
#include <vtos.h>
#include <stdlib.h>
static struct os_fs _os_fs;

static void os_fs_init()
{
	_os_fs.is_update_super = 0;
	_os_fs.open_file_tree = NULL;
	_os_fs.root = NULL;
	_os_fs.super = NULL;
	os_set_init(&_os_fs.files, sizeof(uint32));
}

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
	uint32 path_len = os_str_len(path) + 1;
	if (os_str_cmp(path, "/") == 0)
	{
		buff = (char *)malloc(path_len);
		buff[0] = '/';
		buff[1] = '\0';
	}
	else if (get_file_name(name, path, &index) == 0)
	{
		uint32 *array = (uint32 *)malloc(path_len * sizeof(uint32));
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
			buff = (char *)malloc(path_len);
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
	super->root_id = 0;
	super->property = 1;        //.0 whether open journal
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

static int32 cluster_id_compare2(void *key1, void *key2, void *arg)
{
	uint32 *key = (uint32 *)key1;
	finfo_node *node2 = (finfo_node *)key2;
	if (*key < node2->finfo.cluster_id)
	{
		return -1;
	}
	else if (*key > node2->finfo.cluster_id)
	{
		return 1;
	}
	return 0;
}

static finfo_node *find_finfo_node(finfo_node *finfo_tree, uint32 key)
{
	return (finfo_node *)find_node(&finfo_tree->tree_node_structrue, &key, cluster_id_compare2, NULL);
}

static void super_cluster_flush()
{
	if (is_little_endian())
	{
		cluster_write(SUPER_CLUSTER_ID, (uint8 *)_os_fs.super);
	}
	else
	{
		super_cluster *temp = (super_cluster *)malloc(FS_CLUSTER_SIZE);
		os_mem_cpy(temp, _os_fs.super, FS_CLUSTER_SIZE);

		temp->flag = convert_endian(_os_fs.super->flag);
		temp->root_id = convert_endian(_os_fs.super->root_id);
		temp->property = convert_endian(_os_fs.super->property);
		cluster_write(SUPER_CLUSTER_ID, (uint8 *)temp);
		free(temp);
	}
}

static void flush()
{
	journal_start();
	os_set_iterator *itr = os_set_begin(&_os_fs.files);
	for (; itr != NULL; (itr = os_set_next(&_os_fs.files, itr)))
	{
		uint32 *key = os_set_value(itr);
		finfo_node *node = find_finfo_node(_os_fs.open_file_tree, *key);
		if (node != NULL)
		{
			fnode *n;
			if (node->cluster_id == _os_fs.root->head.node_id)
			{
				n = _os_fs.root;
			}
			else
			{
				n = fnode_load(node->cluster_id);
			}
			os_mem_cpy(&n->finfo[node->index], &node->finfo, sizeof(file_info));
			fnode_flush(n);
			if (n != _os_fs.root)
			{
				fnode_free(n);
			}
		}
	}
	os_set_clear(&_os_fs.files);
	cluster_flush();
	fnodes_flush(_os_fs.root);
	if (_os_fs.is_update_super)
	{
		_os_fs.is_update_super = 0;
		super_cluster_flush();
	}
	journal_end();
}

static void super_cluster_load()
{
	if (is_little_endian())
	{
		cluster_read(SUPER_CLUSTER_ID, (uint8 *)_os_fs.super);
	}
	else
	{
		cluster_read(SUPER_CLUSTER_ID, (uint8 *)_os_fs.super);
		_os_fs.super->flag = convert_endian(_os_fs.super->flag);
		_os_fs.super->root_id = convert_endian(_os_fs.super->root_id);
		_os_fs.super->property = convert_endian(_os_fs.super->property);
	}
}

static void on_move(uint32 id, uint32 index, uint32 key)
{
	finfo_node *f_node = find_finfo_node(_os_fs.open_file_tree, key);
	if (f_node != NULL)
	{
		f_node->cluster_id = id;
		f_node->index = index;
	}
}

static void remove_from_open_file_tree(tree_node_type_def **handle, finfo_node *node)
{
	os_delete_node(handle, &(node->tree_node_structrue));
}

void fs_unloading()
{
	while(_os_fs.open_file_tree != NULL)
	{
		finfo_node *temp = _os_fs.open_file_tree;
		fnode *n = NULL;
		if (temp->cluster_id == _os_fs.root->head.node_id)
		{
			n = _os_fs.root;
		}
		else
		{
			n = fnode_load(temp->cluster_id);
		}
		os_mem_cpy(&n->finfo[temp->index], &temp->finfo, sizeof(file_info));
		fnode_flush(n);
		if (_os_fs.root != n)
		{
			fnode_free(n);
		}
		remove_from_open_file_tree((tree_node_type_def **)(&_os_fs.open_file_tree), temp);
		free(temp);
	}
	flush();
	uninit();
	if (_os_fs.super != NULL)
	{
		free(_os_fs.super);
		_os_fs.super = NULL;
	}
	if (_os_fs.root != NULL)
	{
		free(_os_fs.root);
		_os_fs.root = NULL;
	}
}

static uint32 get_parent_path_and_child_name(char *parent_path, char *child_name)
{
	uint32 flag = 1;
	int32 i = (int32)os_str_len(parent_path) - 1;
	for (; i >= 0; i--)
	{
		if (parent_path[i] != '/')
		{
			break;
		}
	}
	for (; i >= 0; i--)
	{
		if (parent_path[i] == '/')
		{
			flag = 0;
			break;
		}
	}
	if (0 == flag)
	{
		parent_path[i] = '\0';
		i++;
		for (; parent_path[i] != '\0' && parent_path[i] != '/'; i++)
		{
			*child_name = parent_path[i];
			child_name++;
		}
		*child_name = '\0';
	}
	return flag;
}

static fnode *get_file_info(const char *path, uint32 *index)
{
	uint32 flag = 1;
	char name[FS_MAX_NAME_SIZE];
	uint32 i = 0;
	fnode *cur = NULL;
	if (get_file_name(name, path, &i) == 0)
	{
		flag = 0;
		cur = find_from_tree(_os_fs.root, index, name);
		while (get_file_name(name, path, &i) == 0)
		{
			fnode *temp;
			flag = 1;
			if (cur != NULL)
			{
				if (cur->finfo[*index].cluster_id > 0 && cur->finfo[*index].property & 0x00000400 && cur->finfo[*index].file_count > 0)  //判断是否为目录
				{
					uint32 id = cur->finfo[*index].cluster_id;
					if (cur != _os_fs.root)
					{
						fnode_free(cur);
					}
					cur = fnode_load(id);
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
			temp = find_from_tree(cur, index, name);
			if (temp != cur)
			{
				fnode_free(cur);
				cur = temp;
			}
			flag = 0;
		}
	}
	if (flag && cur != _os_fs.root)
	{
		fnode_free(cur);
	}
	if (flag)
	{
		cur = NULL;
	}
	return cur;
}

static uint32 is_root_path(const char *path)
{
	uint32 ret = 0;
	uint32 i = 0;
	if (path[i] == '/')
	{
		ret = 1;
		i++;
		for (; path[i] != '\0'; i++)
		{
			if (path[i] != '/')
			{
				ret = 0;
				break;
			}
		}
	}
	return ret;
}

static fnode *get_parent(char *path, uint32 *idx, char *child_name, uint32 *is_exist)
{
	fnode *ret = NULL;
	*is_exist = 0;
	*idx = FS_MAX_KEY_NUM;
	if (get_parent_path_and_child_name(path, child_name) == 0)
	{
		if (child_name[0] != '\0')
		{
			if (path[0] == '\0' || is_root_path(path))
			{
				ret = _os_fs.root;
				if (finfo_is_exist(ret, child_name))
				{
					*is_exist = 1;
				}
			}
			else
			{
				ret = get_file_info(path, idx);
				if (ret != NULL)
				{
					if (ret->finfo[*idx].cluster_id > 0 && ret->finfo[*idx].property & 0x00000400 && ret->finfo[*idx].file_count > 0)
					{
						fnode *parent = fnode_load(ret->finfo[*idx].cluster_id);
						if (finfo_is_exist(parent, child_name))
						{
							*is_exist = 1;
						}
						fnode_free(parent);
					}
				}
			}
		}
	}
	return ret;
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
	fnode *root = insert_to_btree(_os_fs.root, child);
	if (root != _os_fs.root)
	{
		_os_fs.is_update_super = 1;
		_os_fs.root = root;
		_os_fs.super->root_id = _os_fs.root->head.node_id;
	}
}

static void remove_from_root(const char *name)
{
	fnode *root = remove_from_btree(_os_fs.root, name);
	if (root != _os_fs.root)
	{
		_os_fs.is_update_super = 1;
		_os_fs.root = root;
		_os_fs.super->root_id = _os_fs.root->head.node_id;
	}

}

static void insert_to_parent(file_info *parent, file_info *child)
{
	fnode *root = NULL;
	if (parent->cluster_id > 0)
	{
		root = fnode_load(parent->cluster_id);
	}
	fnode *new_root = insert_to_btree(root, child);
	parent->file_count++;
	if (new_root != root)
	{
		parent->cluster_id = new_root->head.node_id;
		free(new_root);
	}
	fnode_free(root);
}

static void remove_from_parent(file_info *parent_info, fnode *parent, const char *name)
{
	fnode *new_parent = remove_from_btree(parent, name);
	parent_info->file_count--;
	if (new_parent != parent)
	{
		if (new_parent != NULL)
		{
			parent_info->cluster_id = new_parent->head.node_id;
		}
		else
		{
			parent_info->cluster_id = 0;
		}
	}
	if (new_parent != _os_fs.root)
	{
		fnode_free(new_parent);
	}
}

static uint32 do_create_file(const char *path, file_info *finfo)
{
	uint32 flag = 0;
	uint32 index = 0;
	fnode *node = NULL;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		if (NULL == _os_fs.root)
		{
			if (get_file_name(finfo->name, pre_path, &index) == 0 && get_file_name(finfo->name, pre_path, &index) != 0)
			{
				flag = 1;
			}
			free(pre_path);
			pre_path = NULL;
		}
		else
		{
			uint32 is_exist;
			node = get_parent(pre_path, &index, finfo->name, &is_exist);
			free(pre_path);
			pre_path = NULL;
			if (node != NULL)
			{
				if (FS_MAX_KEY_NUM == index && !is_exist)                //父母是根目录
				{
					flag = 1;
				}
				else if (index < FS_MAX_KEY_NUM && !is_exist && node->finfo[index].property & 0x00000400 && node->finfo[index].file_count < 0xffffffff)
				{
					flag = 2;
				}
			}
		}
		if (flag > 0 && os_str_cmp(finfo->name, ".") != 0 && os_str_cmp(finfo->name, "..") != 0 && os_str_find(finfo->name, "/") == -1 && os_str_find(finfo->name, "\\") == -1
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
				add_flush(node);
			}
			if (_os_fs.root != node)
			{
				fnode_free(node);
			}
			return 0;
		}
	}
	return 1;
}

uint32 create_dir(const char *path)
{
	uint32 ret = 1;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property |= 0x00000400;  //设置目录标记
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
	uint32 ret = 1;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property &= (~0x00000400);  //设置文件标记
	finfo->cluster_id = cluster_alloc();
	finfo->cluster_count = 1;
	ret = do_create_file(path, finfo);
	free(finfo);
	if (0 == ret)
	{
		flush();
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
			if (_os_fs.root != NULL)
			{
				id = _os_fs.root->head.node_id;
			}
		}
		else
		{
			uint32 index;
			fnode *node = get_file_info(pre_path, &index);
			if (node != NULL)
			{
				if ((node->finfo[index].property & 0x00000400))
				{
					flag = 1;
					id = node->finfo[index].cluster_id;
				}
				if (node != _os_fs.root)
				{
					fnode_free(node);
				}
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

static uint32 handle_file(const char *path, uint32 (*handle)(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2))
{
	uint32 ret = 1;
	if (_os_fs.root != NULL)
	{
		char *pre_path = pretreat_path(path);
		if (pre_path != NULL)
		{
			uint32 index = 0;
			char name[FS_MAX_NAME_SIZE];
			uint32 is_exist = 0;
			fnode *node = get_parent(pre_path, &index, name, &is_exist);
			free(pre_path);
			pre_path = NULL;
			if (node != NULL)
			{
				if (FS_MAX_KEY_NUM == index && is_exist)                //父母是根目录
				{
					uint32 i;
					fnode *finfo = find_from_tree(_os_fs.root, &i, name);
					if (finfo != NULL)
					{
						if (handle(node, index, _os_fs.root, finfo, i) == 0)
						{
							ret = 0;
						}
						if (finfo != _os_fs.root)
						{
							fnode_free(finfo);
						}
					}
				}
				else if (index < FS_MAX_KEY_NUM && is_exist)
				{
					fnode *parent = NULL;
					uint32 i;
					fnode *finfo = NULL;
					parent = fnode_load(node->finfo[index].cluster_id);
					finfo = find_from_tree(parent, &i, name);
					if (finfo != NULL)
					{
						if (handle(node, index, parent, finfo, i) == 0)
						{
							ret = 0;
						}
						if (finfo != parent)
						{
							fnode_free(finfo);
						}
					}
					fnode_free(parent);
				}
				if (node != _os_fs.root)
				{
					fnode_free(node);
				}
			}
		}
	}
	return ret;
}

static uint32 do_delete_dir(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2)
{
	uint32 ret = 1;
	char name[FS_MAX_NAME_SIZE];
	if (FS_MAX_KEY_NUM == i1)
	{
		if (0 == child->finfo[i2].file_count && child->finfo[i2].property & 0x00000400 && (child->finfo[i2].property & 0x00000200) == 0)
		{
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_root(name);
			ret = 0;
		}
	}
	else
	{
		if (0 == child->finfo[i2].file_count && child->finfo[i2].property & 0x00000400 && (child->finfo[i2].property & 0x00000200) == 0)
		{
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_parent(&parent->finfo[i1], parent_root, name);
			add_flush(parent);
			return 0;
		}
	}
	return ret;
}

uint32 delete_dir(const char *path)
{
	uint32 ret = 1;
	ret = handle_file(path, do_delete_dir);
	if (0 == ret)
	{
		flush();
	}
	return ret;
}

static uint32 sys_do_delete_file(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2)
{
	uint32 ret = 1;
	char name[FS_MAX_NAME_SIZE];
	if (FS_MAX_KEY_NUM == i1)
	{
		if ((child->finfo[i2].property & 0x00000400) == 0 && find_finfo_node(_os_fs.open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			file_data_remove(child->finfo[i2].cluster_id, child->finfo[i2].cluster_count);
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_root(name);
			ret = 0;
		}
	}
	else
	{
		if ((child->finfo[i2].property & 0x00000400) == 0 && find_finfo_node(_os_fs.open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			file_data_remove(child->finfo[i2].cluster_id, child->finfo[i2].cluster_count);
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_parent(&parent->finfo[i1], parent_root, name);
			add_flush(parent);
			return 0;
		}
	}
	return ret;
}

static uint32 do_delete_file(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2)
{
	if ((child->finfo[i2].property & 0x00000200) == 0)
	{
		return sys_do_delete_file(parent, i1, parent_root, child, i2);
	}
	return 1;
}

static uint32 delete_sys_file(const char *path)
{
	uint32 ret = 1;
	ret = handle_file(path, sys_do_delete_file);
	if (0 == ret)
	{
		flush();
	}
	return ret;
}

uint32 delete_file(const char *path)
{
	uint32 ret = 1;
	ret = handle_file(path, do_delete_file);
	if (0 == ret)
	{
		flush();
	}
	return ret;
}

static uint32 sys_do_unlink_file(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2)
{
	uint32 ret = 1;
	char name[FS_MAX_NAME_SIZE];
	if (FS_MAX_KEY_NUM == i1)
	{
		if (find_finfo_node(_os_fs.open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_root(name);
			return 0;
		}
	}
	else
	{
		if (find_finfo_node(_os_fs.open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_parent(&parent->finfo[i1], parent_root, name);
			add_flush(parent);
			return 0;
		}
	}
	return ret;
}

static uint32 do_unlink_file(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2)
{
	if ((child->finfo[i2].property & 0x00000200) == 0)
	{
		return sys_do_unlink_file(parent, i1, parent_root, child, i2);
	}
	return 1;
}

static uint32 unlink_file(const char *path)
{
	return handle_file(path, do_unlink_file);
}

static uint32 sys_unlink_file(const char *path)
{
	return handle_file(path, sys_do_unlink_file);
}

uint32 move_file(const char *dest, const char *src)
{
	uint32 ret = 1;
	char *pre_src = pretreat_path(src);
	if (pre_src != NULL)
	{
		uint32 index;
		fnode *node = get_file_info(pre_src, &index);
		free(pre_src);
		if (node != NULL)
		{
			file_info *finfo = (file_info *)malloc(sizeof(file_info));
			os_mem_cpy(finfo, &node->finfo[index], sizeof(file_info));
			if (node != _os_fs.root)
			{
				fnode_free(node);
			}
			if (do_create_file(dest, finfo) == 0)
			{
				free(finfo);
				if (unlink_file(src) == 0)
				{
					ret = 0;
					flush();
				}
				else
				{
					sys_unlink_file(dest);
				}
			}
		}
	}
	return ret;
}

static int32 cluster_id_compare(void *key1, void *key2, void *arg)
{
	finfo_node *node1 = (finfo_node *)key1;
	finfo_node *node2 = (finfo_node *)key2;
	if (node1->finfo.cluster_id < node2->finfo.cluster_id)
	{
		return -1;
	}
	else if (node1->finfo.cluster_id > node2->finfo.cluster_id)
	{
		return 1;
	}
	return 0;
}

static void insert_to_open_file_tree(tree_node_type_def **handle, finfo_node *node)
{
	os_insert_node(handle, &node->tree_node_structrue, cluster_id_compare, NULL);
}

file_obj *open_file(const char *path, uint32 flags)
{
	file_obj *file = NULL;
	char *pre_path = pretreat_path(path);
	if(pre_path != NULL)
	{
		uint32 index;
		fnode *node = NULL;
		if (flags & FS_CREATE)
		{
			delete_file(pre_path);
			create_file(pre_path);
		}
		node = get_file_info(pre_path, &index);
		if (node != NULL && (node->finfo[index].property & 0x00000400) == 0)  //检查文件属性
		{
			finfo_node *f_node = find_finfo_node(_os_fs.open_file_tree, node->finfo[index].cluster_id);
			if (f_node != NULL)
			{
				if (f_node->count < 0xffffffff)
				{
					f_node->count++;
				}
				else
				{
					if (node != _os_fs.root)
					{
						fnode_free(node);
					}
					free(pre_path);
					return file;
				}
			}
			else
			{
				f_node = (finfo_node *)malloc(sizeof(finfo_node));
				f_node->count = 1;
				f_node->cluster_id = node->head.node_id;
				f_node->index = index;
				os_mem_cpy(&f_node->finfo, &node->finfo[index], sizeof(file_info));
				insert_to_open_file_tree((tree_node_type_def **)(&_os_fs.open_file_tree), f_node);
			}
			if (node != _os_fs.root)
			{
				fnode_free(node);
			}
			free(pre_path);
			file = (file_obj *)malloc(sizeof(file_obj));
			file->flags = flags;
			if (flags & FS_APPEND)
			{
				file->index = f_node->finfo.size;
			}
			else
			{
				file->index = 0;
			}
			file->node = f_node;
		}
	}
	return file;
}

void close_file(file_obj *file)
{
	fnode *n = NULL;
	if (file->node->cluster_id == _os_fs.root->head.node_id)
	{
		n = _os_fs.root;
	}
	else
	{
		n = fnode_load(file->node->cluster_id);
	}
	os_mem_cpy(&n->finfo[file->node->index], &file->node->finfo, sizeof(file_info));
	fnode_flush(n);
	if (_os_fs.root != n)
	{
		fnode_free(n);
	}
	file->node->count--;
	if (0 == file->node->count)
	{
		remove_from_open_file_tree((tree_node_type_def **)(&_os_fs.open_file_tree), file->node);
		free(file->node);
	}
	free(file);
	flush();
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

static uint32 sys_write_file(file_obj *file, void *data, uint32 len)
{
	if ((file->flags & FS_WRITE) && (file->node->finfo.property & 0x00000092)) //判断是否具有写权限
	{
		if (len > 0)
		{
			uint32 temp = file->node->finfo.cluster_count;
			if (file->index + len <= FS_CLUSTER_SIZE && 1 == file->node->finfo.cluster_count)  //按照小文件方式写入
			{
				if (data != NULL)
				{
					little_file_data_write(file->node->finfo.cluster_id, file->index, data, len);
				}
			}
			else
			{
				file->node->finfo.cluster_id = file_data_write(file->node->finfo.cluster_id, &file->node->finfo.cluster_count, file->index, data, len);
			}

			if (data != NULL && file->index + len > file->node->finfo.size)
			{
				file->node->finfo.size = file->index + len;
			}
			file->index += len;
			file->node->finfo.modif_time = os_get_time();
			if (temp != file->node->finfo.cluster_count)
			{
				os_set_insert(&_os_fs.files, &file->node->finfo.cluster_id);
			}
			return len;
		}
	}
	return 0;
}

uint32 write_file(file_obj *file, void *data, uint32 len)
{
	if ((file->node->finfo.property & 0x00000200) == 0) //判断是否具有写权限
	{
		return sys_write_file(file, data, len);
	}
	return 0;
}

static uint32 create_sys_file(const char *path)
{
	uint32 ret = 1;
	file_info *finfo = (file_info *)malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property &= (~0x00000400);  //设置文件标记
	finfo->property |= 0x00000200;  //设置系统只写
	finfo->cluster_id = cluster_alloc();
	finfo->cluster_count = 1;
	ret = do_create_file(path, finfo);
	free(finfo);
	if (0 == ret)
	{
		flush();
	}
	return ret;
}

uint32 fs_loading()
{
	while (sizeof(fnode) != FS_CLUSTER_SIZE);
	os_fs_init();
	os_cluster_init();
	os_dentry_init();
	register_on_move_info(on_move);
	cluster_manager_load();
	_os_fs.super = (super_cluster *)malloc(FS_CLUSTER_SIZE);
	super_cluster_load();
	if (os_str_cmp(_os_fs.super->name, "emfs") == 0)
	{
		if (_os_fs.super->root_id > 0)
		{
			_os_fs.root = fnode_load(_os_fs.super->root_id);
		}
		else
		{
			_os_fs.root = NULL;
		}
		register_callback(create_sys_file, delete_sys_file, sys_write_file, read_file);
		return 0;
	}
	uninit();
	if (_os_fs.super != NULL)
	{
		free(_os_fs.super);
		_os_fs.super = NULL;
	}
	if (_os_fs.root != NULL)
	{
		free(_os_fs.root);
		_os_fs.root = NULL;
	}
	return 1;
}

void fs_formatting()
{
	file_obj *file = NULL;
	while (sizeof(fnode) != FS_CLUSTER_SIZE);
	os_fs_init();
	os_cluster_init();
	os_dentry_init();
	register_on_move_info(on_move);
	cluster_manager_init();
	_os_fs.super = (super_cluster *)malloc(FS_CLUSTER_SIZE);
	super_cluster_init(_os_fs.super);
	super_cluster_flush();
	register_callback(create_sys_file, delete_sys_file, sys_write_file, read_file);
	journal_init();
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

uint64 tell_file(file_obj *file)
{
	return file->index;
}



