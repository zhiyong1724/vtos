#include "fs/os_fs.h"
#include "base/os_string.h"
#include "fs/os_file.h"
#include "vtos.h"
#define MAX_MOUNT_COUNT 10
static os_fs *_file_systems[MAX_MOUNT_COUNT] = {NULL, };
static void os_fs_init(os_fs *fs)
{
	fs->is_update_super = 0;
	fs->open_file_tree = NULL;
	fs->root = NULL;
	fs->super = NULL;
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

static void super_cluster_init(super_cluster *super)
{
	super->magic = 0xaa55a55a;
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
	info->property = 0x000001b6; //.10 dir or file;.9 only sys w;.876 user r,w,x;.543 group r,w,x;.210 other r,w,x
	info->file_count = 0;
	info->name[0] = '\0';
}

static int8 cluster_id_compare2(void *key1, void *key2, void *arg)
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

static void super_cluster_flush(os_fs *fs)
{
	if (is_little_endian())
	{
		cluster_write(SUPER_CLUSTER_ID, (uint8 *)fs->super, &fs->cluster);
	}
	else
	{
		super_cluster *temp = (super_cluster *)os_malloc(FS_CLUSTER_SIZE);
		os_mem_cpy(temp, fs->super, FS_CLUSTER_SIZE);

		temp->magic = convert_endian(fs->super->magic);
		temp->root_id = convert_endian(fs->super->root_id);
		temp->property = convert_endian(fs->super->property);
		cluster_write(SUPER_CLUSTER_ID, (uint8 *)temp, &fs->cluster);
		os_free(temp);
	}
}

static void flush(os_fs *fs)
{
	if (fs->super->property & 0x00000001)
	{
		journal_start(&fs->journal);
	}
	cluster_flush(&fs->cluster);
	fnodes_flush(fs->root, &fs->dentry);
	if (fs->is_update_super)
	{
		fs->is_update_super = 0;
		super_cluster_flush(fs);
	}
	if (fs->super->property & 0x00000001)
	{
		journal_end(&fs->journal);
	}
}

static void super_cluster_load(os_fs *fs)
{
	if (is_little_endian())
	{
		cluster_read(SUPER_CLUSTER_ID, (uint8 *)fs->super, &fs->cluster);
	}
	else
	{
		cluster_read(SUPER_CLUSTER_ID, (uint8 *)fs->super, &fs->cluster);
		fs->super->magic = convert_endian(fs->super->magic);
		fs->super->root_id = convert_endian(fs->super->root_id);
		fs->super->property = convert_endian(fs->super->property);
	}
}

static void on_move(uint32 id, uint32 index, uint32 key, uint32 dev_id)
{
	os_fs *fs = _file_systems[dev_id];
	if (fs != NULL)
	{
		finfo_node *f_node = find_finfo_node(fs->open_file_tree, key);
		if (f_node != NULL)
		{
			f_node->cluster_id = id;
			f_node->index = index;
		}
	}
}

static void remove_from_open_file_tree(tree_node_type_def **handle, finfo_node *node)
{
	os_delete_node(handle, &(node->tree_node_structrue));
}

static uint32 get_parent_path_and_child_name(const char *path, char *parent_path, char *child_name)
{
	uint32 flag = 1;
	int32 i = (int32)os_str_len(path) - 1;
	for (; i >= 0; i--)
	{
		if (path[i] != '/')
		{
			break;
		}
	}
	for (; i >= 0; i--)
	{
		if (path[i] == '/')
		{
			flag = 0;
			break;
		}
	}
	if (0 == flag)
	{
		int32 j;
		for (j = 0; j < i; j++)
		{
			parent_path[j] = path[j];
		}
		parent_path[j] = '\0';
		i++;
		for (; path[i] != '\0' && path[i] != '/'; i++)
		{
			*child_name = path[i];
			child_name++;
		}
		*child_name = '\0';
	}
	return flag;
}

static fnode *get_file_info(const char *path, uint32 *index, os_fs *fs)
{
	uint32 flag = 1;
	char name[FS_MAX_NAME_SIZE];
	uint32 i = 0;
	fnode *cur = NULL;
	if (get_file_name(name, path, &i) == 0)
	{
		flag = 0;
		cur = find_from_tree(fs->root, index, name, &fs->dentry);
		while (get_file_name(name, path, &i) == 0)
		{
			fnode *temp;
			flag = 1;
			if (cur != NULL)
			{
				if (cur->finfo[*index].cluster_id > 0 && cur->finfo[*index].property & 0x00000400 && cur->finfo[*index].file_count > 0)  //判断是否为目录
				{
					uint32 id = cur->finfo[*index].cluster_id;
					if (cur != fs->root)
					{
						fnode_free(cur, &fs->dentry);
					}
					cur = fnode_load(id, &fs->dentry);
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
			temp = find_from_tree(cur, index, name, &fs->dentry);
			if (temp != cur)
			{
				fnode_free(cur, &fs->dentry);
				cur = temp;
			}
			flag = 0;
		}
	}
	if (flag && cur != fs->root)
	{
		fnode_free(cur, &fs->dentry);
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

static fnode *get_parent(const char *path, uint32 *idx, char *child_name, uint32 *is_exist, os_fs *fs)
{
	fnode *ret = NULL;
	uint32 path_len = os_str_len(path) + 1;
	char *parent_path = (char *)os_malloc(path_len);
	*is_exist = 0;
	*idx = FS_MAX_KEY_NUM;
	if (get_parent_path_and_child_name(path, parent_path, child_name) == 0)
	{
		if (child_name[0] != '\0')
		{
			if (parent_path[0] == '\0' || is_root_path(parent_path))
			{
				ret = fs->root;
				if (finfo_is_exist(ret, child_name, &fs->dentry))
				{
					*is_exist = 1;
				}
			}
			else
			{
				ret = get_file_info(parent_path, idx, fs);
				if (ret != NULL)
				{
					if (ret->finfo[*idx].cluster_id > 0 && ret->finfo[*idx].property & 0x00000400 && ret->finfo[*idx].file_count > 0)
					{
						fnode *parent = fnode_load(ret->finfo[*idx].cluster_id, &fs->dentry);
						if (finfo_is_exist(parent, child_name, &fs->dentry))
						{
							*is_exist = 1;
						}
						fnode_free(parent, &fs->dentry);
					}
				}
			}
		}
	}
	os_free(parent_path);
	return ret;
}

static dir_obj *init_dir_obj(uint32 id)
{
	dir_obj *dir = (dir_obj *)os_malloc(sizeof(dir_obj));
	dir->cur = NULL;
	dir->id = id;
	dir->index = 0;
	dir->parent = NULL;
	return dir;
}

static void insert_to_root(file_info *child, os_fs *fs)
{
	fnode *root = insert_to_btree(fs->root, child, &fs->dentry);
	if (root != fs->root)
	{
		fs->is_update_super = 1;
		fs->root = root;
		fs->super->root_id = fs->root->head.node_id;
	}
}

static void remove_from_root(const char *name, os_fs *fs)
{
	fnode *root = remove_from_btree(fs->root, name, &fs->dentry);
	if (root != fs->root)
	{
		fs->is_update_super = 1;
		fs->root = root;
		fs->super->root_id = fs->root->head.node_id;
	}

}

static void insert_to_parent(file_info *parent, file_info *child, os_fs *fs)
{
	fnode *root = NULL;
	if (parent->cluster_id > 0)
	{
		root = fnode_load(parent->cluster_id, &fs->dentry);
	}
	fnode *new_root = insert_to_btree(root, child, &fs->dentry);
	parent->file_count++;
	if (new_root != root)
	{
		parent->cluster_id = new_root->head.node_id;
		os_free(new_root);
	}
	fnode_free(root, &fs->dentry);
}

static void remove_from_parent(file_info *parent_info, fnode *parent, const char *name, os_fs *fs)
{
	fnode *new_parent = remove_from_btree(parent, name, &fs->dentry);
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
	if (new_parent != fs->root)
	{
		fnode_free(new_parent, &fs->dentry);
	}
}

static uint32 do_create_file(const char *path, file_info *finfo, os_fs *fs)
{
	uint32 flag = 0;
	uint32 index = 0;
	fnode *node = NULL;
	if (NULL == fs->root)
	{
		if (get_file_name(finfo->name, path, &index) == 0 && get_file_name(finfo->name, path, &index) != 0)
		{
			flag = 1;
		}
	}
	else
	{
		uint32 is_exist;
		node = get_parent(path, &index, finfo->name, &is_exist, fs);
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
			insert_to_root(finfo, fs);
			if (fs->root != node)
			{
				fnode_free(node, &fs->dentry);
			}
		}
		else
		{
			insert_to_parent(&node->finfo[index], finfo, fs);
			add_flush(node, &fs->dentry);
		}
		return 0;
	}
	return 1;
}

uint32 fs_create_dir(const char *path, os_fs *fs)
{
	uint32 ret = 1;
	file_info *finfo = (file_info *)os_malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property |= 0x00000400;  //设置目录标记
	ret = do_create_file(path, finfo, fs);
	os_free(finfo);
	if (0 == ret)
	{
		flush(fs);
	}
	return ret;
}

uint32 fs_create_file(const char *path, os_fs *fs)
{
	uint32 ret = 1;
	file_info *finfo = (file_info *)os_malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property &= (~0x00000400);  //设置文件标记
	finfo->cluster_id = cluster_alloc(&fs->cluster);
	finfo->cluster_count = 1;
	ret = do_create_file(path, finfo, fs);
	os_free(finfo);
	if (0 == ret)
	{
		flush(fs);
	}
	return ret;
}

dir_obj *fs_open_dir(const char *path, os_fs *fs)
{
	uint32 id = 0;
	uint32 flag = 0;
	if (os_str_cmp(path, "/") == 0)
	{
		flag = 1;
		if (fs->root != NULL)
		{
			id = fs->root->head.node_id;
		}
	}
	else
	{
		uint32 index;
		fnode *node = get_file_info(path, &index, fs);
		if (node != NULL)
		{
			if ((node->finfo[index].property & 0x00000400))
			{
				flag = 1;
				id = node->finfo[index].cluster_id;
			}
			if (node != fs->root)
			{
				fnode_free(node, &fs->dentry);
			}
		}
	}
	if (flag)
	{
		dir_obj *dir = (dir_obj *)os_malloc(sizeof(dir_obj));
		dir->cur = NULL;
		dir->index = 0;
		dir->parent = NULL;
		dir->id = id;
		return dir;
	}
	return NULL;
}

void fs_close_dir(dir_obj *dir)
{
	if (dir->cur != NULL)
	{
		os_free(dir->cur);
	}
	if (dir->parent != NULL)
	{
		os_free(dir->parent);
	}
	os_free(dir);
}

uint32 fs_read_dir(file_info *finfo, dir_obj *dir, os_fs *fs)
{
	if (dir->id > 0)
	{
		return query_finfo(finfo, &dir->id, &dir->parent, &dir->cur, &dir->index, &fs->dentry);
	}
	return 1;
}

static uint32 handle_file(const char *path, uint32(*handle)(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2, os_fs *fs), os_fs *fs)
{
	uint32 ret = 1;
	if (fs->root != NULL)
	{
		uint32 index = 0;
		char name[FS_MAX_NAME_SIZE];
		uint32 is_exist = 0;
		fnode *node = get_parent(path, &index, name, &is_exist, fs);
		if (node != NULL)
		{
			if (FS_MAX_KEY_NUM == index && is_exist)                //父母是根目录
			{
				uint32 i;
				fnode *finfo = find_from_tree(fs->root, &i, name, &fs->dentry);
				if (finfo != NULL)
				{
					if (handle(node, index, fs->root, finfo, i, fs) == 0)
					{
						ret = 0;
					}
					if (finfo != fs->root)
					{
						fnode_free(finfo, &fs->dentry);
					}
				}
			}
			else if (index < FS_MAX_KEY_NUM && is_exist)
			{
				fnode *parent = NULL;
				uint32 i;
				fnode *finfo = NULL;
				parent = fnode_load(node->finfo[index].cluster_id, &fs->dentry);
				finfo = find_from_tree(parent, &i, name, &fs->dentry);
				if (finfo != NULL)
				{
					if (handle(node, index, parent, finfo, i, fs) == 0)
					{
						ret = 0;
					}
					if (finfo != parent)
					{
						fnode_free(finfo, &fs->dentry);
					}
				}
				fnode_free(parent, &fs->dentry);
			}
			if (node != fs->root)
			{
				fnode_free(node, &fs->dentry);
			}
		}
	}
	return ret;
}

static uint32 do_delete_dir(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2, os_fs *fs)
{
	uint32 ret = 1;
	char name[FS_MAX_NAME_SIZE];
	if (FS_MAX_KEY_NUM == i1)
	{
		if (0 == child->finfo[i2].file_count && child->finfo[i2].property & 0x00000400 && (child->finfo[i2].property & 0x00000200) == 0)
		{
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_root(name, fs);
			ret = 0;
		}
	}
	else
	{
		if (0 == child->finfo[i2].file_count && child->finfo[i2].property & 0x00000400 && (child->finfo[i2].property & 0x00000200) == 0)
		{
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_parent(&parent->finfo[i1], parent_root, name, fs);
			add_flush(parent, &fs->dentry);
			return 0;
		}
	}
	return ret;
}

uint32 fs_delete_dir(const char *path, os_fs *fs)
{
	uint32 ret = 1;
	ret = handle_file(path, do_delete_dir, fs);
	if (0 == ret)
	{
		flush(fs);
	}
	return ret;
}

static uint32 sys_do_delete_file(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2, os_fs *fs)
{
	uint32 ret = 1;
	char name[FS_MAX_NAME_SIZE];
	if (FS_MAX_KEY_NUM == i1)
	{
		if ((child->finfo[i2].property & 0x00000400) == 0 && find_finfo_node(fs->open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			file_data_remove(child->finfo[i2].cluster_id, child->finfo[i2].cluster_count, &fs->cluster);
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_root(name, fs);
			ret = 0;
		}
	}
	else
	{
		if ((child->finfo[i2].property & 0x00000400) == 0 && find_finfo_node(fs->open_file_tree, child->finfo[i2].cluster_id) == NULL)
		{
			file_data_remove(child->finfo[i2].cluster_id, child->finfo[i2].cluster_count, &fs->cluster);
			os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
			remove_from_parent(&parent->finfo[i1], parent_root, name, fs);
			add_flush(parent, &fs->dentry);
			return 0;
		}
	}
	return ret;
}

static uint32 do_delete_file(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2, os_fs *fs)
{
	if ((child->finfo[i2].property & 0x00000200) == 0)
	{
		return sys_do_delete_file(parent, i1, parent_root, child, i2, fs);
	}
	return 1;
}

static uint32 delete_sys_file(const char *path, os_fs *fs)
{
	uint32 ret = 1;
	ret = handle_file(path, sys_do_delete_file, fs);
	if (0 == ret)
	{
		flush(fs);
	}
	return ret;
}

uint32 fs_delete_file(const char *path, os_fs *fs)
{
	uint32 ret = 1;
	ret = handle_file(path, do_delete_file, fs);
	if (0 == ret)
	{
		flush(fs);
	}
	return ret;
}

static uint32 sys_do_unlink_file(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2, os_fs *fs)
{
	char name[FS_MAX_NAME_SIZE];
	if (FS_MAX_KEY_NUM == i1)
	{
		os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
		remove_from_root(name, fs);
		return 0;
	}
	else
	{
		os_str_cpy(name, child->finfo[i2].name, FS_MAX_NAME_SIZE);
		remove_from_parent(&parent->finfo[i1], parent_root, name, fs);
		add_flush(parent, &fs->dentry);
		return 0;
	}
	return 1;
}

static uint32 do_unlink_file(fnode *parent, uint32 i1, fnode *parent_root, fnode *child, uint32 i2, os_fs *fs)
{
	if ((child->finfo[i2].property & 0x00000200) == 0 && find_finfo_node(fs->open_file_tree, child->finfo[i2].cluster_id) == NULL)
	{
		return sys_do_unlink_file(parent, i1, parent_root, child, i2, fs);
	}
	return 1;
}

static uint32 unlink_file(const char *path, os_fs *fs)
{
	return handle_file(path, do_unlink_file, fs);
}

static uint32 sys_unlink_file(const char *path, os_fs *fs)
{
	return handle_file(path, sys_do_unlink_file, fs);
}

uint32 fs_move_file(const char *dest, const char *src, os_fs *fs)
{
	uint32 ret = 1;
	uint32 index;
	fnode *node = get_file_info(src, &index, fs);
	if (node != NULL)
	{
		file_info *finfo = (file_info *)os_malloc(sizeof(file_info));
		os_mem_cpy(finfo, &node->finfo[index], sizeof(file_info));
		if (node != fs->root)
		{
			fnode_free(node, &fs->dentry);
		}
		if (do_create_file(dest, finfo, fs) == 0)
		{
			os_free(finfo);
			if (unlink_file(src, fs) == 0)
			{
				ret = 0;
				flush(fs);
			}
			else
			{
				sys_unlink_file(dest, fs);
			}
		}
	}
	return ret;
}

static int8 cluster_id_compare(void *key1, void *key2, void *arg)
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

file_obj *fs_open_file(const char *path, uint32 flags, os_fs *fs)
{
	file_obj *file = NULL;
	uint32 index;
	fnode *node = NULL;
	if (flags & FS_CREATE)
	{
		fs_delete_file(path, fs);
		fs_create_file(path, fs);
	}
	node = get_file_info(path, &index, fs);
	if (node != NULL && (node->finfo[index].property & 0x00000400) == 0)  //检查文件属性
	{
		finfo_node *f_node = find_finfo_node(fs->open_file_tree, node->finfo[index].cluster_id);
		if (f_node != NULL)
		{
			if (f_node->count < 0xffffffff)
			{
				f_node->count++;
			}
			else
			{
				if (node != fs->root)
				{
					fnode_free(node, &fs->dentry);
				}
				return file;
			}
		}
		else
		{
			f_node = (finfo_node *)os_malloc(sizeof(finfo_node));
			f_node->count = 1;
			f_node->cluster_id = node->head.node_id;
			f_node->index = index;
			f_node->flag = 0;
			os_mem_cpy(&f_node->finfo, &node->finfo[index], sizeof(file_info));
			insert_to_open_file_tree((tree_node_type_def **)(&fs->open_file_tree), f_node);
		}
		if (node != fs->root)
		{
			fnode_free(node, &fs->dentry);
		}
		file = (file_obj *)os_malloc(sizeof(file_obj));
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
	return file;
}

void fs_close_file(file_obj *file, os_fs *fs)
{
	if (file->node->flag > 0)
	{
		fnode *n = NULL;
		flush(fs);
		if (file->node->cluster_id == fs->root->head.node_id)
		{
			n = fs->root;
		}
		else
		{
			n = fnode_load(file->node->cluster_id, &fs->dentry);
		}
		os_mem_cpy(&n->finfo[file->node->index], &file->node->finfo, sizeof(file_info));
		fnode_flush(n, &fs->dentry);
		if (fs->root != n)
		{
			fnode_free(n, &fs->dentry);
		}
		file->node->flag = 0;
	}
	file->node->count--;
	if (0 == file->node->count)
	{
		remove_from_open_file_tree((tree_node_type_def **)(&fs->open_file_tree), file->node);
		os_free(file->node);
	}
	os_free(file);
}

uint32 fs_read_file(file_obj *file, void *data, uint32 len, os_fs *fs)
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
				little_file_data_read(file->node->finfo.cluster_id, file->index, data, len, &fs->cluster);
			}
			else
			{
				file_data_read(file->node->finfo.cluster_id, file->node->finfo.cluster_count, file->index, data, len, &fs->cluster);
				file->index += len;
			}
			return len;
		}
	}
	return 0;
}

static uint32 sys_write_file(file_obj *file, void *data, uint32 len, os_fs *fs)
{
	if ((file->flags & FS_WRITE) && (file->node->finfo.property & 0x00000092)) //判断是否具有写权限
	{
		if (len > 0)
		{
			if (file->index + len <= FS_CLUSTER_SIZE && 1 == file->node->finfo.cluster_count)  //按照小文件方式写入
			{
				if (data != NULL)
				{
					little_file_data_write(file->node->finfo.cluster_id, file->index, data, len, &fs->cluster);
				}
			}
			else
			{
				file->node->finfo.cluster_id = file_data_write(file->node->finfo.cluster_id, &file->node->finfo.cluster_count, file->index, data, len, &fs->cluster);
			}

			if (file->index + len > file->node->finfo.size)
			{
				file->node->finfo.size = file->index + len;
				file->node->flag = 1;
			}
			file->index += len;
			file->node->finfo.modif_time = os_get_time();
			return len;
		}
	}
	return 0;
}

uint32 fs_write_file(file_obj *file, void *data, uint32 len, os_fs *fs)
{
	if ((file->node->finfo.property & 0x00000200) == 0) //判断是否具有写权限
	{
		return sys_write_file(file, data, len, fs);
	}
	return 0;
}

static uint32 create_sys_file(const char *path, os_fs *fs)
{
	uint32 ret = 1;
	file_info *finfo = (file_info *)os_malloc(sizeof(file_info));
	file_info_init(finfo);
	finfo->property &= (~0x00000400);  //设置文件标记
	finfo->property |= 0x00000200;  //设置系统只写
	finfo->cluster_id = cluster_alloc(&fs->cluster);
	finfo->cluster_count = 1;
	ret = do_create_file(path, finfo, fs);
	os_free(finfo);
	if (0 == ret)
	{
		flush(fs);
	}
	return ret;
}

os_fs *fs_mount(uint32 dev_id, uint32 formatting)
{
	os_fs *fs = NULL;
	if (dev_id >= 0 && dev_id < MAX_MOUNT_COUNT && NULL == _file_systems[dev_id] && sizeof(fnode) == FS_CLUSTER_SIZE)
	{
		fs = (os_fs *)os_malloc(sizeof(os_fs));
		_file_systems[dev_id] = fs;
		os_fs_init(fs);
		os_cluster_init(&fs->cluster, dev_id);
		os_dentry_init(&fs->dentry, on_move, &fs->cluster);
		os_journal_init(&fs->journal, create_sys_file, sys_write_file, fs);
		set_journal(&fs->journal, &fs->cluster);
		if (1 == formatting)
		{
			cluster_manager_init(&fs->cluster);
			fs->super = (super_cluster *)os_malloc(FS_CLUSTER_SIZE);
			super_cluster_init(fs->super);
			super_cluster_flush(fs);
			journal_create(&fs->journal);
		}
		else
		{
			fs->super = (super_cluster *)os_malloc(FS_CLUSTER_SIZE);
			super_cluster_load(fs);
			if (os_str_cmp(fs->super->name, "emfs") == 0)
			{
				cluster_manager_load(&fs->cluster);
				fs->root = fnode_load(fs->super->root_id, &fs->dentry);
				if (fs->super->property & 0x00000001)
				{
					if (restore_from_journal(&fs->journal) == 0)
					{
						fnode_free(fs->root, &fs->dentry);
						fs->root = fnode_load(fs->super->root_id, &fs->dentry);
					}
				}
				return fs;
			}
			os_cluster_uninit(&fs->cluster);
			os_dentry_uninit(&fs->dentry);
			os_journal_uninit(&fs->journal);
			if (fs->super != NULL)
			{
				os_free(fs->super);
				fs->super = NULL;
			}
			if (fs->root != NULL)
			{
				os_free(fs->root);
				fs->root = NULL;
			}
			os_free(fs);
		}
	}
	return fs;
}

void fs_unmount(os_fs *fs)
{
	flush(fs);
	while (fs->open_file_tree != NULL)
	{
		finfo_node *temp = fs->open_file_tree;
		if (temp->flag > 0)
		{
			fnode *n = NULL;
			if (temp->cluster_id == fs->root->head.node_id)
			{
				n = fs->root;
			}
			else
			{
				n = fnode_load(temp->cluster_id, &fs->dentry);
			}
			os_mem_cpy(&n->finfo[temp->index], &temp->finfo, sizeof(file_info));
			fnode_flush(n, &fs->dentry);
			if (fs->root != n)
			{
				fnode_free(n, &fs->dentry);
			}
		}
		remove_from_open_file_tree((tree_node_type_def **)(&fs->open_file_tree), temp);
		os_free(temp);
	}
	os_cluster_uninit(&fs->cluster);
	os_dentry_uninit(&fs->dentry);
	os_journal_uninit(&fs->journal);
	if (fs->super != NULL)
	{
		os_free(fs->super);
		fs->super = NULL;
	}
	if (fs->root != NULL)
	{
		os_free(fs->root);
		fs->root = NULL;
	}
	os_free(fs);
}

void fs_formatting(os_fs *fs)
{
	uint32 dev_id = fs->cluster.dev_id;
	while (fs->open_file_tree != NULL)
	{
		finfo_node *temp = fs->open_file_tree;
		remove_from_open_file_tree((tree_node_type_def **)(&fs->open_file_tree), temp);
		os_free(temp);
	}
	os_cluster_uninit(&fs->cluster);
	os_dentry_uninit(&fs->dentry);
	os_journal_uninit(&fs->journal);
	if (fs->super != NULL)
	{
		os_free(fs->super);
		fs->super = NULL;
	}
	if (fs->root != NULL)
	{
		os_free(fs->root);
		fs->root = NULL;
	}

	os_fs_init(fs);
	os_cluster_init(&fs->cluster, dev_id);
	os_dentry_init(&fs->dentry, on_move, &fs->cluster);
	os_journal_init(&fs->journal, create_sys_file, sys_write_file, fs);
	set_journal(&fs->journal, &fs->cluster);

	cluster_manager_init(&fs->cluster);
	fs->super = (super_cluster *)os_malloc(FS_CLUSTER_SIZE);
	super_cluster_init(fs->super);
	super_cluster_flush(fs);
	journal_create(&fs->journal);
}

uint32 fs_seek_file(file_obj *file, int64 offset, uint32 fromwhere)
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

uint64 fs_tell_file(file_obj *file)
{
	return file->index;
}

void fs_open_journal(os_fs *fs)
{
	fs->super->property |= 0x00000001;
	super_cluster_flush(fs);
}

void fs_close_journal(os_fs *fs)
{
	fs->super->property &= (~0x00000001);
	super_cluster_flush(fs);
}



