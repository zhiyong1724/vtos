#include "fs/os_dentry.h"
#include "base/os_string.h"
#include "fs/os_cluster.h"
#include "vtos.h"
#include <stdlib.h>
static struct os_dentry _os_dentry;
void os_dentry_init()
{
	_os_dentry.move_callback = NULL;
	os_map_init(&_os_dentry.fnodes, sizeof(uint32), sizeof(void *));
}

void fnode_flush(fnode *node)
{
	if (is_little_endian())
	{
		cluster_write(node->head.node_id, (uint8 *)node);
	}
	else
	{
		uint32 i = 0;
		fnode *temp = (fnode *)malloc(sizeof(fnode));
		os_mem_cpy(temp, node, sizeof(fnode));
		temp->head.node_id = convert_endian(node->head.node_id);
		temp->head.num = convert_endian(node->head.num);
		temp->head.leaf = convert_endian(node->head.leaf);
		temp->head.next = convert_endian(node->head.next);
		for (i = 0; i < node->head.num + 1; i++)
		{
			temp->head.pointers[i] = convert_endian(node->head.pointers[i]);
		}

		for (i = 0; i < node->head.num; i++)
		{
			temp->finfo[i].cluster_id = convert_endian(node->finfo[i].cluster_id);
			temp->finfo[i].cluster_count = convert_endian(node->finfo[i].cluster_count);
			temp->finfo[i].creator = convert_endian(node->finfo[i].creator);
			temp->finfo[i].modifier = convert_endian(node->finfo[i].modifier);
			temp->finfo[i].property = convert_endian(node->finfo[i].property);
			temp->finfo[i].file_count = convert_endian(node->finfo[i].file_count);
			temp->finfo[i].size = convert_endian64(node->finfo[i].size);
			temp->finfo[i].create_time = convert_endian64(node->finfo[i].create_time);
			temp->finfo[i].modif_time = convert_endian64(node->finfo[i].modif_time);
		}
		cluster_write(node->head.node_id, (uint8 *)temp);
		free(temp);
	}
}

void fnodes_flush(fnode *root)
{
	os_map_iterator *itr = os_map_begin(&_os_dentry.fnodes);
	for (; itr != NULL; (itr = os_map_next(&_os_dentry.fnodes, itr)))
	{
		fnode **pp = (fnode **)os_map_second(&_os_dentry.fnodes, itr);
		fnode_flush(*pp);
		if (*pp != root)
		{
			free(*pp);
		}
	}
	os_map_clear(&_os_dentry.fnodes);
}

fnode *fnode_load(uint32 id)
{
	fnode *node = NULL;
	os_map_iterator *itr = os_map_find(&_os_dentry.fnodes, &id);
	if (itr != NULL)
	{
		fnode **pp = (fnode **)os_map_second(&_os_dentry.fnodes, itr);
		node = *pp;
	}
	else
	{
		node = (fnode *)malloc(sizeof(fnode));
		cluster_read(id, (uint8 *)node);
		if (!is_little_endian())
		{
			uint32 i = 0;
			node->head.node_id = convert_endian(node->head.node_id);
			node->head.num = convert_endian(node->head.num);
			node->head.leaf = convert_endian(node->head.leaf);
			node->head.next = convert_endian(node->head.next);
			for (i = 0; i < node->head.num + 1; i++)
			{
				node->head.pointers[i] = convert_endian(node->head.pointers[i]);
			}
			for (i = 0; i < node->head.num; i++)
			{
				node->finfo[i].cluster_id = convert_endian(node->finfo[i].cluster_id);
				node->finfo[i].cluster_count = convert_endian(node->finfo[i].cluster_count);
				node->finfo[i].creator = convert_endian(node->finfo[i].creator);
				node->finfo[i].modifier = convert_endian(node->finfo[i].modifier);
				node->finfo[i].property = convert_endian(node->finfo[i].property);
				node->finfo[i].file_count = convert_endian(node->finfo[i].file_count);
				node->finfo[i].size = convert_endian64(node->finfo[i].size);
				node->finfo[i].create_time = convert_endian64(node->finfo[i].create_time);
				node->finfo[i].modif_time = convert_endian64(node->finfo[i].modif_time);
			}
		}
	}
	return node;
}

uint32 fnode_free(fnode *node)
{
	if (node != NULL)
	{
		os_map_iterator *itr = os_map_find(&_os_dentry.fnodes, &node->head.node_id);
		if (NULL == itr)
		{
			free(node);
			return 0;
		}
	}
	return 1;
}

void insert_to_fnodes(fnode *node)
{
	if (node != NULL)
	{
		os_map_insert(&_os_dentry.fnodes, &node->head.node_id, &node);
	}
}

//初始化node
static fnode *init_node(fnode *node)
{
	uint32 i = 0;
	for (; i < FS_MAX_KEY_NUM + 1; i++)
	{
		node->head.pointers[i] = 0;
	}
	node->head.node_id = 0;
	node->head.leaf = 0;
	node->head.num = 0;
	node->head.next = 0;
	return node;
}

//初始化leaf
static fnode *init_leaf(fnode *node)
{
	uint32 i = 0;
	for (; i < FS_MAX_KEY_NUM + 1; i++)
	{
		node->head.pointers[i] = 0;
	}
	node->head.node_id = 0;
	node->head.leaf = 1;
	node->head.num = 0;
	node->head.next = 0;
	return node;
}

//创建node
static fnode *make_node()
{
	uint32 cluster_id;
	fnode *node = (fnode *)malloc(sizeof(fnode));
	init_node(node);
	cluster_id = cluster_alloc();
	node->head.node_id = cluster_id;
	return node;
}

//创建leaf
static fnode *make_leaf()
{
	uint32 cluster_id;
	fnode *node = (fnode *)malloc(sizeof(fnode));
	init_leaf(node);
	cluster_id = cluster_alloc();
	node->head.node_id = cluster_id;
	return node;
}

//分裂孩子节点
static fnode *split_child(fnode *root, uint32 i, fnode *child)
{
	fnode *new_node;
	uint32 j;
	//创建一个新的节点
	new_node = make_node();
	new_node->head.num = FS_MAX_KEY_NUM / 2;
	new_node->head.leaf = child->head.leaf;

	//复制child最后的FS_MAX_KEY_NUM / 2的keys到新节点
	for (j = 0; j < FS_MAX_KEY_NUM / 2; j++)
	{
		os_mem_cpy(&new_node->finfo[j], &child->finfo[j + FS_MAX_KEY_NUM / 2], sizeof(file_info));
		_os_dentry.move_callback(new_node->head.node_id, j, child->finfo[j + FS_MAX_KEY_NUM / 2].cluster_id);
	}

	//复制child最后的FS_MAX_KEY_NUM / 2 + 1的孩子到新节点
	if (child->head.leaf == 0)
	{
		for (j = 0; j <= FS_MAX_KEY_NUM / 2; j++)
			new_node->head.pointers[j] = child->head.pointers[j + FS_MAX_KEY_NUM / 2];
	}

	//减少child的num
	child->head.num = FS_MAX_KEY_NUM / 2 - 1;
	//把new_node增加到root节点中
	for (j = root->head.num; j >= i + 1; j--)
		root->head.pointers[j + 1] = root->head.pointers[j];
	root->head.pointers[i + 1] = new_node->head.node_id;
	for (j = root->head.num; j > i; j--)
	{
		os_mem_cpy(&root->finfo[j], &root->finfo[j - 1], sizeof(file_info));
		_os_dentry.move_callback(root->head.node_id, j, root->finfo[j - 1].cluster_id);
	}

	//复制child的中间key到root
	os_mem_cpy(&root->finfo[i], &child->finfo[FS_MAX_KEY_NUM / 2 - 1], sizeof(file_info));
	_os_dentry.move_callback(root->head.node_id, i, child->finfo[FS_MAX_KEY_NUM / 2 - 1].cluster_id);

	//增加root计数
	root->head.num++;
	return new_node;
}

static void insert_non_full(fnode *root, file_info *finfo)
{
	uint32 i = root->head.num;
	//如果是叶子节点 
	if (root->head.leaf == 1)
	{
		//找到插入的位置
		for (; i > 0 && os_str_cmp(root->finfo[i - 1].name, finfo->name) > 0; i--)
		{
			os_mem_cpy(&root->finfo[i], &root->finfo[i - 1], sizeof(file_info));
			_os_dentry.move_callback(root->head.node_id, i, root->finfo[i - 1].cluster_id);
		}
		os_mem_cpy(&root->finfo[i], finfo, sizeof(file_info));
		root->head.num++; 
	}
	else
	{
		fnode *new_node = NULL;
		fnode *split_node = NULL;
		//找到插入的位置
		while (i > 0 && os_str_cmp(root->finfo[i - 1].name, finfo->name) > 0)
			i--;

		new_node = fnode_load(root->head.pointers[i]);
		insert_to_fnodes(new_node);
		if (new_node->head.num == FS_MAX_KEY_NUM)
		{
			//如果孩子满  
			split_node = split_child(root, i, new_node);

			split_node->head.next = new_node->head.next;
			new_node->head.next = split_node->head.node_id;

			if (os_str_cmp(root->finfo[i].name, finfo->name) < 0)
			{
				new_node = split_node;
			}
		}
		insert_non_full(new_node, finfo);
		if (split_node != NULL)
		{
			fnode_flush(split_node);
			free(split_node);
		}
	}
}

//插入新的节点
fnode *insert_to_btree(fnode *root, file_info *finfo)
{
	//如果树是空树，创建一棵树 
	if (root == NULL)
	{
		root = make_leaf();
		root->head.num = 1;
		os_mem_cpy(root->finfo, finfo, sizeof(file_info));
		fnode_flush(root);
	}
	else // 如果树不为空  
	{
		//如果当前节点满，则分裂
		if (root->head.num == FS_MAX_KEY_NUM)
		{
			fnode *new_node;
			fnode *split_node;
			//生成新的节点
			new_node = make_node();
			//把root作为新节点的孩子  
			new_node->head.pointers[0] = root->head.node_id;

			//分裂新节点的孩子节点
			split_node = split_child(new_node, 0, root);

			//把两个孩子连接起来
			root->head.next = split_node->head.node_id;

			//决定插入到哪个孩子中
			if (os_str_cmp(new_node->finfo[0].name, finfo->name) < 0)
			{
				insert_non_full(split_node, finfo);
			}
			else
			{
				insert_non_full(root, finfo);
			}
			fnode_flush(split_node);
			fnode_flush(new_node);
			free(split_node);
			//改变root
			root = new_node;
		}
		else
		{
			insert_non_full(root, finfo);
		}

	}
	return root;
}

//融合两个孩子节点
static void merge(fnode *root, fnode *child, fnode *sibling, uint32 idx)
{
	uint32 i = 0;

	//拷贝内容
	os_mem_cpy(&child->finfo[FS_MAX_KEY_NUM / 2 - 1], &root->finfo[idx], sizeof(file_info));
	_os_dentry.move_callback(child->head.node_id, FS_MAX_KEY_NUM / 2 - 1, root->finfo[idx].cluster_id);
	for (i = 0; i < sibling->head.num; ++i)
	{
		os_mem_cpy(&child->finfo[i + FS_MAX_KEY_NUM / 2], &sibling->finfo[i], sizeof(file_info));
		_os_dentry.move_callback(child->head.node_id, i + FS_MAX_KEY_NUM / 2, sibling->finfo[i].cluster_id);
	}

	//拷贝指针
	if (!child->head.leaf)
	{
		for (i = 0; i <= sibling->head.num; ++i)
			child->head.pointers[i + FS_MAX_KEY_NUM / 2] = sibling->head.pointers[i];
	}

	//移动父节点的内容
	for (i = idx + 1; i < root->head.num; ++i)
	{
		os_mem_cpy(&root->finfo[i - 1], &root->finfo[i], sizeof(file_info));
		_os_dentry.move_callback(root->head.node_id, i - 1, root->finfo[i].cluster_id);
	}

	//移动父节点的指针
	for (i = idx + 2; i <= root->head.num; ++i)
		root->head.pointers[i - 1] = root->head.pointers[i];

	//更新父节点和孩子节点计数  
	child->head.num += sibling->head.num + 1;
	root->head.num--;
	child->head.next = sibling->head.next;
	//释放簇 
	cluster_free(sibling->head.node_id);
}

//在节点中找到key的位置
static uint32 find_key(fnode *root, const char *name)
{
	uint32 idx = 0;
	while (idx < root->head.num && os_str_cmp(root->finfo[idx].name, name) < 0)
		++idx;
	return idx;
}

//查找文件
fnode *find_from_tree(fnode *root, uint32 *index, const char *name)
{
	fnode *cur = root;
	for(; cur != NULL;)
	{
		for (*index = 0; *index < cur->head.num; (*index)++)
		{
			int32 cmp = os_str_cmp(cur->finfo[*index].name, name);
			if (0 == cmp)
			{
				return cur;
			}
			else if (cmp > 0)
			{
				break;
			}
		}

		if (!cur->head.leaf && cur->head.pointers[*index] != 0)
		{
			uint32 id = cur->head.pointers[*index];
			if (cur != root)
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
	if (cur != root)
	{
		fnode_free(cur);
	}
	return NULL;
}

//判断文件是否存在
uint32 finfo_is_exist(fnode *root, const char *name)
{
	uint32 index;
	fnode *cur = root;
	for (; cur != NULL;)
	{
		for (index = 0; index < cur->head.num; (index)++)
		{
			int32 cmp = os_str_cmp(cur->finfo[index].name, name);
			if (0 == cmp)
			{
				if (cur != root)
				{
					fnode_free(cur);
				}
				return 1;
			}
			else if (cmp > 0)
			{
				break;
			}
		}

		if (!cur->head.leaf && cur->head.pointers[index] != 0)
		{
			uint32 id = cur->head.pointers[index];
			if (cur != root)
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
	if (cur != NULL && cur != root)
	{
		fnode_free(cur);
	}
	return 0;
}

//遍历文件
uint32 query_finfo(file_info *finfo, uint32 *id, fnode **parent, fnode **cur, uint32 *index)
{
	if (NULL == *parent)
	{
		*parent = fnode_load(*id);
		*cur = fnode_load(*id);
	}
	if (*cur != NULL)
	{
		if (*index == (*cur)->head.num)
		{
			*index = 0;
			*id = (*cur)->head.next;
			if (*id != 0)
			{
				fnode_free(*cur);
				*cur = fnode_load(*id);
			}
			else
			{
				*id = (*parent)->head.pointers[0];
				if (*id != 0)
				{
					fnode_free(*parent);
					fnode_free(*cur);
					*parent = fnode_load(*id);
					*cur = fnode_load(*id);
				}
				else
				{
					fnode_free(*parent);
					fnode_free(*cur);
					*parent = NULL;
					*cur = NULL;
					return 1;
				}
			}
		}
	}
	if (*cur != NULL)
	{
		os_mem_cpy(finfo, &(*cur)->finfo[*index], sizeof(file_info));
		(*index)++;
		return 0;
	}
	return 1;
}

//找到左边最大的key
static void get_pred(file_info *finfo, fnode *root)
{ 
	fnode *cur = root;
	uint32 id = cur->head.pointers[cur->head.num];
	while (!cur->head.leaf)
	{
		if (cur != root)
		{
			fnode_free(cur);
		}
		cur = fnode_load(id);
		id = cur->head.pointers[cur->head.num];
	}
	os_mem_cpy(finfo, &cur->finfo[cur->head.num - 1], sizeof(file_info));
	if (cur != root)
	{
		fnode_free(cur);
	}
}
//找到右边最小的key
static void get_succ(file_info *finfo, fnode *root)
{
	fnode *cur = root;
	uint32 id = cur->head.pointers[0];
	while (!cur->head.leaf)
	{
		if (cur != root)
		{
			fnode_free(cur);
		}
		cur = fnode_load(id);
		id = cur->head.pointers[0];
	}
	os_mem_cpy(finfo, &cur->finfo[0], sizeof(file_info));
	if (cur != root)
	{
		fnode_free(cur);
	}
}

//从叶子节点中删除
static void remove_from_leaf(fnode *root, uint32 idx)
{ 
	uint32 i;
	for (i = idx + 1; i < root->head.num; ++i)
	{
		os_mem_cpy(&root->finfo[i - 1], &root->finfo[i], sizeof(file_info));
		_os_dentry.move_callback(root->head.node_id, i - 1, root->finfo[i].cluster_id);
	}
	root->head.num--;
	return;
}

//从非叶子节点中删除 
static void remove_from_non_leaf(fnode *root, uint32 idx)
{
	fnode *node;
	file_info *temp = (file_info *)malloc(sizeof(file_info));
	os_mem_cpy(temp, &root->finfo[idx], sizeof(file_info));

	node = fnode_load(root->head.pointers[idx]);
	insert_to_fnodes(node);
	if (node->head.num >= FS_MAX_KEY_NUM / 2)
	{
		file_info *pred = (file_info *)malloc(sizeof(file_info));
		get_pred(pred, node);
		os_mem_cpy(&root->finfo[idx], pred, sizeof(file_info));
		_os_dentry.move_callback(root->head.node_id, idx, pred->cluster_id);
		remove_from_btree(node, pred->name);
		free(pred);
	}
	else
	{
		fnode *nnode = fnode_load(root->head.pointers[idx + 1]);
		insert_to_fnodes(nnode);
		if (nnode->head.num >= FS_MAX_KEY_NUM / 2)
		{
			file_info *succ = (file_info *)malloc(sizeof(file_info));
			get_succ(succ, nnode);
			os_mem_cpy(&root->finfo[idx], succ, sizeof(file_info));
			_os_dentry.move_callback(root->head.node_id, idx, succ->cluster_id);
			remove_from_btree(nnode, succ->name);
			free(succ);
		}
		else
		{
			merge(root, node, nnode, idx);
			remove_from_btree(node, temp->name);
		}
	}
	free(temp);
}

//从前面的兄弟借值
static void borrow_from_prev(fnode *root, fnode *child, fnode *sibling, uint32 idx)
{
	uint32 i;
	//移动内容
	for (i = child->head.num; i > 0; --i)
	{
		os_mem_cpy(&child->finfo[i], &child->finfo[i - 1], sizeof(file_info));
		_os_dentry.move_callback(child->head.node_id, i, child->finfo[i - 1].cluster_id);
	}

	//移动指针
	if (!child->head.leaf)
	{
		for (i = child->head.num + 1; i > 0; --i)
			child->head.pointers[i] = child->head.pointers[i - 1];
	}

	os_mem_cpy(&child->finfo[0], &root->finfo[idx - 1], sizeof(file_info));
	_os_dentry.move_callback(child->head.node_id, 0, root->finfo[idx - 1].cluster_id);
	if (!root->head.leaf)
		child->head.pointers[0] = sibling->head.pointers[sibling->head.num];

	os_mem_cpy(&root->finfo[idx - 1], &sibling->finfo[sibling->head.num - 1], sizeof(file_info));
	_os_dentry.move_callback(root->head.node_id, idx - 1, sibling->finfo[sibling->head.num - 1].cluster_id);

	child->head.num += 1;
	sibling->head.num -= 1;
}

//从后面的兄弟借值
static void borrow_from_next(fnode *root, fnode *child, fnode *sibling, uint32 idx)
{
	uint32 i;
	os_mem_cpy(&child->finfo[child->head.num], &root->finfo[idx], sizeof(file_info));
	_os_dentry.move_callback(child->head.node_id, child->head.num, root->finfo[idx].cluster_id);

	if (!child->head.leaf)
		child->head.pointers[child->head.num + 1] = sibling->head.pointers[0];

	os_mem_cpy(&root->finfo[idx], &sibling->finfo[0], sizeof(file_info));
	_os_dentry.move_callback(root->head.node_id, idx, sibling->finfo[0].cluster_id);

	for (i = 1; i < sibling->head.num; ++i)
	{
		os_mem_cpy(&sibling->finfo[i - 1], &sibling->finfo[i], sizeof(file_info));
		_os_dentry.move_callback(sibling->head.node_id, i - 1, sibling->finfo[i].cluster_id);
	}

	if (!sibling->head.leaf)
	{
		for (i = 1; i <= sibling->head.num; ++i)
			sibling->head.pointers[i - 1] = sibling->head.pointers[i];
	}

	child->head.num += 1;
	sibling->head.num -= 1;
}
//值不足要填充足够的值
static void fill(fnode *root, fnode *child, uint32 idx)
{
	fnode *left = NULL;
	if (idx != 0)
	{
		left = fnode_load(root->head.pointers[idx - 1]);
		insert_to_fnodes(left);
		if (left->head.num >= FS_MAX_KEY_NUM / 2)
		{
			borrow_from_prev(root, child, left, idx);
			return;
		}

	}

	if (idx != root->head.num)
	{
		fnode *right;
		right = fnode_load(root->head.pointers[idx + 1]);
		insert_to_fnodes(right);
		if (right->head.num >= FS_MAX_KEY_NUM / 2)
		{
			borrow_from_next(root, child, right, idx);
		}
		else
		{
			merge(root, child, right, idx);
		}
	}	
	else
	{
		if (NULL == left)
		{
			left = fnode_load(root->head.pointers[idx - 1]);
			insert_to_fnodes(left);
		}
		merge(root, left, child, idx - 1);
	}
}

fnode *remove_from_btree(fnode *root, const char *name)
{
	if (!root)
	{
		return root;
	}

	{
		uint32 idx = find_key(root, name);

		//如果这个info在父节点中
		if (idx < root->head.num && os_str_cmp(root->finfo[idx].name, name) == 0)
		{
			if (root->head.leaf)
				remove_from_leaf(root, idx);
			else
				remove_from_non_leaf(root, idx);
		}
		else
		{
			uint32 flag;
			fnode *child;
			//如果是叶子节点，则表明要删除的记录不存在 
			if (root->head.leaf)
			{
				return root;
			}

			flag = ((idx == root->head.num) ? 1 : 0);

			child = fnode_load(root->head.pointers[idx]);
			insert_to_fnodes(child);
			if (child->head.num < FS_MAX_KEY_NUM / 2)
			{
				fill(root, child, idx);
			}

			if (flag && idx > root->head.num)
			{
				fnode *sibling = fnode_load(root->head.pointers[idx - 1]);
				insert_to_fnodes(sibling);
				remove_from_btree(sibling, name);
			}
			else
				remove_from_btree(child, name);

		}
	}
	//如果root没有值，删除根节点，他的孩子作为根节点
	if (root->head.num == 0)
	{
		fnode *tmp = root;
		if (root->head.leaf)
			root = NULL;
		else
			root = fnode_load(root->head.pointers[0]);
		cluster_free(tmp->head.node_id);

	}
	return root;
}

void register_on_move_info(on_move_info call_back)
{
	_os_dentry.move_callback = call_back;
}