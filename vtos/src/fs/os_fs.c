#include "fs/os_fs.h"
#include "lib/os_string.h"
#include "fs/os_cluster.h"
#include "fs/os_file.h"
#include "fs/os_journal.h"
#include <stdio.h>
#include <vtos.h>
#include <stdlib.h>
static super_cluster *_super = NULL;
static fnode *_root = NULL;
static finfo_node *_open_file_tree = NULL;

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
	super->root_id = ROOT_CLUSTER_ID;
	super->backup_id = ROOT_CLUSTER_ID + 1;
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

static void super_cluster_flush()
{
	if (is_little_endian())
	{
		cluster_write(SUPER_CLUSTER_ID, (uint8 *)_super);
	}
	else
	{
		super_cluster *temp = (super_cluster *)malloc(FS_CLUSTER_SIZE);
		os_mem_cpy(temp, _super, FS_CLUSTER_SIZE);

		temp->flag = convert_endian(_super->flag);
		temp->root_id = convert_endian(_super->root_id);
		temp->backup_id = convert_endian(_super->backup_id);
		temp->property = convert_endian(_super->property);
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
		_super->property = convert_endian(_super->property);
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
		file_info *temp = (file_info  *)malloc(FS_CLUSTER_SIZE);
		os_mem_cpy(temp, finfo, FS_CLUSTER_SIZE);
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

static uint32 file_info_flush_return(uint32 id, file_info *finfo)
{
	uint32 ret = 0;
	if (is_little_endian())
	{
		if (cluster_write_return(id, (uint8 *)finfo) != 0)
		{
			ret = 1;
		}
	}
	else
	{
		file_info *temp = (file_info  *)malloc(FS_CLUSTER_SIZE);
		os_mem_cpy(temp, finfo, FS_CLUSTER_SIZE);
		temp->cluster_id = convert_endian(finfo->cluster_id);
		temp->cluster_count = convert_endian(finfo->cluster_count);
		temp->creator = convert_endian(finfo->creator);
		temp->modifier = convert_endian(finfo->modifier);
		temp->property = convert_endian(finfo->property);
		temp->file_count = convert_endian(finfo->file_count);
		temp->size = convert_endian64(finfo->size);
		temp->create_time = convert_endian64(finfo->create_time);
		temp->modif_time = convert_endian64(finfo->modif_time);
		if (cluster_write_return(id, (uint8 *)temp) != 0)
		{
			ret = 1;
		}
		free(temp);
	}
	return ret;
}

static file_info *file_info_load(uint32 id)
{
	file_info *finfo = (file_info *)malloc(FS_CLUSTER_SIZE);
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

static file_info *file_info_load_return(uint32 id)
{
	file_info *finfo = (file_info *)malloc(FS_CLUSTER_SIZE);
	if (cluster_read_return(id, (uint8 *)finfo) != 0)
	{
		free(finfo);
		return NULL;
	}
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

static void on_move(uint32 id, uint32 index, uint32 key)
{
	finfo_node *f_node = find_finfo_node(_open_file_tree, key);
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
	while(_open_file_tree != NULL)
	{
		finfo_node *temp = _open_file_tree;
		fnode *n = NULL;
		if (temp->cluster_id == _root->head.node_id)
		{
			n = _root;
		}
		else
		{
			n = fnode_load(temp->cluster_id);
		}
		os_mem_cpy(&n->finfo[temp->index], &temp->finfo, sizeof(file_info));
		fnode_flush(n);
		if (_root != n)
		{
			free(n);
		}
		remove_from_open_file_tree((tree_node_type_def **)(&_open_file_tree), temp);
		free(temp);
	}
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
		cur = find_from_tree(_root, index, name);
		while (get_file_name(name, path, &i) == 0)
		{
			fnode *temp;
			flag = 1;
			if (cur != NULL)
			{
				if (cur->finfo[*index].cluster_id > 0 && cur->finfo[*index].property & 0x00000400 && cur->finfo[*index].file_count > 0)  //判断是否为目录
				{
					uint32 id = cur->finfo[*index].cluster_id;
					if (cur != _root)
					{
						free(cur);
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
				free(cur);
				cur = temp;
			}
			flag = 0;
		}
	}
	if (flag && cur != NULL && cur != _root)
	{
		free(cur);
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
				ret = _root;
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
						free(parent);
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
	fnode *root = insert_to_btree(_root, child);
	if (root != _root)
	{
		file_info *finfo = file_info_load(_super->root_id);
		_root = root;
		finfo->cluster_id = _root->head.node_id;
		for (; file_info_flush_return(_super->root_id, finfo) != 0;)
		{
			_super->root_id = cluster_alloc();
		}
		for (; file_info_flush_return(_super->backup_id, finfo) != 0;)
		{
			_super->backup_id = cluster_alloc();
		}
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
		for (; file_info_flush_return(_super->root_id, finfo) != 0;)
		{
			_super->root_id = cluster_alloc();
		}
		for (; file_info_flush_return(_super->backup_id, finfo) != 0;)
		{
			_super->backup_id = cluster_alloc();
		}
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
	return new_parent;
}

static uint32 do_create_file(const char *path, file_info *finfo)
{
	uint32 ret = 1;
	uint32 flag = 0;
	uint32 index = 0;
	fnode *node = NULL;
	fnode *temp = _root;
	char *pre_path = pretreat_path(path);
	if (pre_path != NULL)
	{
		if (NULL == _root)
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
				fnode_flush(node);
			}

			ret = 0;
		}

		if (node != NULL && node != temp)
		{
			free(node);
		}
		if (pre_path != NULL)
		{
			free(pre_path);
		}
	}
	return ret;
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
			if (_root != NULL)
			{
				id = _root->head.node_id;
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
				if (node != _root)
				{
					free(node);
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

static uint32 handle_file(const char *path, uint32 (*handle)(fnode *parent, uint32 i1, fnode **parent_root, fnode *child, uint32 i2))
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
			free(pre_path);
			pre_path = NULL;
			if (node != NULL)
			{
				fnode *temp = _root;
				if (FS_MAX_KEY_NUM == index && is_exist)                //父母是根目录
				{
					uint32 i;
					fnode *finfo = find_from_tree(_root, &i, name);
					if (finfo != NULL)
					{
						if (handle(node, index, NULL, finfo, i) == 0)
						{
							ret = 0;
						}
						if (finfo != temp)
						{
							free(finfo);
						}
					}
				}
				else if (index < FS_MAX_KEY_NUM && is_exist)
				{
					fnode *parent = NULL;
					fnode *parent_temp = NULL;
					uint32 i;
					fnode *finfo = NULL;
					parent = fnode_load(node->finfo[index].cluster_id);
					parent_temp = parent;
					finfo = find_from_tree(parent, &i, name);
					if (finfo != NULL)
					{
						if (handle(node, index, &parent, finfo, i) == 0)
						{
							ret = 0;
						}
						if (finfo != temp && finfo != parent_temp)
						{
							free(finfo);
						}
					}
					if (parent != NULL)
					{
						free(parent);
					}
				}
				if (node != NULL && node != temp)
				{
					free(node);
				}
			}
			if (pre_path != NULL)
			{
				free(pre_path);
			}
		}
	}
	return ret;
}

static uint32 do_delete_dir(fnode *parent, uint32 i1, fnode **parent_root, fnode *child, uint32 i2)
{
	if (FS_MAX_KEY_NUM == i1)
	{
		if (0 == child->finfo[i2].file_count && child->finfo[i2].property & 0x00000400 && (child->finfo[i2].property & 0x00000200) == 0)
		{
			remove_from_root(child->finfo[i2].name);
			return 0;
		}
	}
	else
	{
		if (0 == child->finfo[i2].file_count && child->finfo[i2].property & 0x00000400 && (child->finfo[i2].property & 0x00000200) == 0)
		{
			*parent_root = remove_from_parent(&parent->finfo[i1], *parent_root, child->finfo[i2].name);
			fnode_flush(parent);
			return 0;
		}
	}
	return 1;
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

static uint32 sys_do_delete_file(fnode *parent, uint32 i1, fnode **parent_root, fnode *child, uint32 i2)
{
	if (FS_MAX_KEY_NUM == i1)
	{
		if ((child->finfo[i2].property & 0x00000400) == 0 && find_finfo_node(_open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			file_data_remove(child->finfo[i2].cluster_id, child->finfo[i2].cluster_count);
			remove_from_root(child->finfo[i2].name);
			return 0;
		}
	}
	else
	{
		if ((child->finfo[i2].property & 0x00000400) == 0 && find_finfo_node(_open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			file_data_remove(child->finfo[i2].cluster_id, child->finfo[i2].cluster_count);
			*parent_root = remove_from_parent(&parent->finfo[i1], *parent_root, child->finfo[i2].name);
			fnode_flush(parent);
			return 0;
		}
	}
	return 1;
}

static uint32 do_delete_file(fnode *parent, uint32 i1, fnode **parent_root, fnode *child, uint32 i2)
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

static uint32 sys_do_unlink_file(fnode *parent, uint32 i1, fnode **parent_root, fnode *child, uint32 i2)
{
	if (FS_MAX_KEY_NUM == i1)
	{
		if (find_finfo_node(_open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			remove_from_root(child->finfo[i2].name);
			return 0;
		}
	}
	else
	{
		if (find_finfo_node(_open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			*parent_root = remove_from_parent(&parent->finfo[i1], *parent_root, child->finfo[i2].name);
			fnode_flush(parent);
			return 0;
		}
	}
	return 1;
}

static uint32 do_unlink_file(fnode *parent, uint32 i1, fnode **parent_root, fnode *child, uint32 i2)
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

static uint32 move_sys_file(const char *dest, const char *src)
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
			if (node != _root)
			{
				free(node);
			}
			if (do_create_file(dest, finfo) == 0)
			{
				free(finfo);
				if (sys_unlink_file(src) == 0)
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
			if (node != _root)
			{
				free(node);
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

static void insert_to_open_file_tree(tree_node_type_def **handle, finfo_node *node)
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
			finfo_node *f_node = find_finfo_node(_open_file_tree, node->finfo[index].cluster_id);
			if (f_node != NULL)
			{
				if (f_node->count < 0xffffffff)
				{
					f_node->count++;
				}
				else
				{
					if (node != _root)
					{
						free(node);
					}
					free(pre_path);
					return file;
				}
			}
			else
			{
				f_node = (finfo_node *)malloc(sizeof(finfo_node));
				f_node->count = 1;
				f_node->key = node->finfo[index].cluster_id;
				f_node->cluster_id = node->head.node_id;
				f_node->index = index;
				os_mem_cpy(&f_node->finfo, &node->finfo[index], sizeof(file_info));
				insert_to_open_file_tree((tree_node_type_def **)(&_open_file_tree), f_node);
			}
			if (node != _root)
			{
				free(node);
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
	if (file->node->cluster_id == _root->head.node_id)
	{
		n = _root;
	}
	else
	{
		n = fnode_load(file->node->cluster_id);
	}
	os_mem_cpy(&n->finfo[file->node->index], &file->node->finfo, sizeof(file_info));
	fnode_flush(n);
	if (_root != n)
	{
		free(n);
	}
	file->node->count--;
	if (0 == file->node->count)
	{
		remove_from_open_file_tree((tree_node_type_def **)(&_open_file_tree), file->node);
		free(file->node);
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

static uint32 sys_write_file(file_obj *file, void *data, uint32 len)
{
	if ((file->flags & FS_WRITE) && (file->node->finfo.property & 0x00000092)) //判断是否具有写权限
	{
		if (len > 0)
		{
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
			flush();
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
	file_info *finfo = NULL;
	while (sizeof(fnode) != FS_CLUSTER_SIZE);
	cluster_controler_init();
	cluster_manager_load();
	_super = (super_cluster *)malloc(FS_CLUSTER_SIZE);
	super_cluster_load();
	if (os_str_cmp(_super->name, "emfs") == 0)
	{
		finfo = file_info_load_return(_super->root_id);
		if (NULL == finfo)
		{
			finfo = file_info_load(_super->backup_id);
		}
		if (finfo->cluster_id > 0)
		{
			_root = fnode_load(finfo->cluster_id);
		}
		else
		{
			_root = NULL;
		}
		free(finfo);
		register_on_move_info(on_move);
		register_callback(create_sys_file, delete_sys_file, sys_write_file, move_sys_file);
		return 0;
	}
	fs_unloading();
	return 1;
}

void fs_formatting()
{
	file_info *finfo = NULL;
	file_obj *file = NULL;
	while (sizeof(fnode) != FS_CLUSTER_SIZE);
	cluster_controler_init();
	cluster_manager_init();
	_super = (super_cluster *)malloc(FS_CLUSTER_SIZE);
	super_cluster_init(_super);
	finfo = (file_info *)malloc(FS_CLUSTER_SIZE);
	file_info_init(finfo);
	finfo->property |= 0x00000600;       //设置只是系统写权限和目录标记
	os_str_cpy(finfo->name, "/", FS_MAX_NAME_SIZE);
	finfo->cluster_id = 0;
	file_info_flush(ROOT_CLUSTER_ID, finfo);
	file_info_flush(ROOT_CLUSTER_ID + 1, finfo);
	free(finfo);
	super_cluster_flush();
	register_callback(create_sys_file, delete_sys_file, sys_write_file, move_sys_file);
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



