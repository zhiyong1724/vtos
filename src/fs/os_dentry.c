#include "fs/os_dentry.h"
#include "base/os_string.h"
#include "fs/os_cluster.h"
#include "vtos.h"
void os_dentry_init(os_dentry *dentry, on_move_info call_back, os_cluster *cluster)
{
	dentry->move_callback = call_back;
	os_map_init(&dentry->fnodes, sizeof(uint32), sizeof(fnode_h));
	dentry->cluster = cluster;
}

void os_dentry_uninit(os_dentry *dentry)
{
	dentry->move_callback = NULL;
	os_map_clear(&dentry->fnodes);
	dentry->cluster = NULL;
}

void fnode_flush(fnode *node, os_dentry *dentry)
{
	if (is_little_endian())
	{
		cluster_write(node->head.node_id, (uint8 *)node, dentry->cluster);
	}
	else
	{
		uint32 i = 0;
		fnode *temp = (fnode *)os_malloc(sizeof(fnode));
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
		cluster_write(node->head.node_id, (uint8 *)temp, dentry->cluster);
		os_free(temp);
	}
}

void fnodes_flush(fnode *root, os_dentry *dentry)
{
	os_map_iterator *itr = os_map_begin(&dentry->fnodes);
	for (; itr != NULL; (itr = os_map_next(&dentry->fnodes, itr)))
	{
		fnode_h *handle = (fnode_h *)os_map_second(&dentry->fnodes, itr);
		if (1 == handle->flag)
		{
			fnode_flush(handle->node, dentry);
		}
		if (handle->node != root)
		{
			os_free(handle->node);
		}
	}
	os_map_clear(&dentry->fnodes);
	insert_to_fnodes(root, dentry);
}

fnode *fnode_load(uint32 id, os_dentry *dentry)
{
	fnode *node = NULL;
	os_map_iterator *itr = os_map_find(&dentry->fnodes, &id);
	if (itr != NULL)
	{
		fnode_h *handle = (fnode_h *)os_map_second(&dentry->fnodes, itr);
		handle->count++;
		node = handle->node;
	}
	else
	{
		node = (fnode *)os_malloc(sizeof(fnode));
		cluster_read(id, (uint8 *)node, dentry->cluster);
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
		insert_to_fnodes(node, dentry);
	}
	return node;
}

uint32 fnode_free(fnode *node, os_dentry *dentry)
{
	if (node != NULL)
	{
		os_map_iterator *itr = os_map_find(&dentry->fnodes, &node->head.node_id);
		if (itr != NULL)
		{
			fnode_h *handle = (fnode_h *)os_map_second(&dentry->fnodes, itr);
			handle->count--;
			if (0 == handle->count && 0 == handle->flag)
			{
				os_map_erase(&dentry->fnodes, itr);
				os_free(node);
				return 0;
			}
		}
	}
	return 1;
}

uint32 fnode_free_f(fnode *node, os_dentry *dentry)
{
	if (node != NULL)
	{
		os_map_iterator *itr = os_map_find(&dentry->fnodes, &node->head.node_id);
		if (itr != NULL)
		{
			os_map_erase(&dentry->fnodes, itr);
			os_free(node);
			return 0;
		}
	}
	return 1;
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
static fnode *make_node(os_dentry *dentry)
{
	uint32 cluster_id;
	fnode *node = (fnode *)os_malloc(sizeof(fnode));
	init_node(node);
	cluster_id = cluster_alloc(dentry->cluster);
	node->head.node_id = cluster_id;
	return node;
}

//创建leaf
static fnode *make_leaf(os_dentry *dentry)
{
	uint32 cluster_id;
	fnode *node = (fnode *)os_malloc(sizeof(fnode));
	init_leaf(node);
	cluster_id = cluster_alloc(dentry->cluster);
	node->head.node_id = cluster_id;
	return node;
}

//分裂孩子节点
static fnode *split_child(fnode *root, uint32 i, fnode *child, os_dentry *dentry)
{
	fnode *new_node;
	uint32 j;
	//创建一个新的节点
	new_node = make_node(dentry);
	new_node->head.num = FS_MAX_KEY_NUM / 2;
	new_node->head.leaf = child->head.leaf;

	//复制child最后的FS_MAX_KEY_NUM / 2的keys到新节点
	for (j = 0; j < FS_MAX_KEY_NUM / 2; j++)
	{
		os_mem_cpy(&new_node->finfo[j], &child->finfo[j + FS_MAX_KEY_NUM / 2], sizeof(file_info));
		dentry->move_callback(new_node->head.node_id, j, child->finfo[j + FS_MAX_KEY_NUM / 2].cluster_id, dentry->cluster->dev_id);
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
		dentry->move_callback(root->head.node_id, j, root->finfo[j - 1].cluster_id, dentry->cluster->dev_id);
	}

	//复制child的中间key到root
	os_mem_cpy(&root->finfo[i], &child->finfo[FS_MAX_KEY_NUM / 2 - 1], sizeof(file_info));
	dentry->move_callback(root->head.node_id, i, child->finfo[FS_MAX_KEY_NUM / 2 - 1].cluster_id, dentry->cluster->dev_id);

	//增加root计数
	root->head.num++;
	add_flush(root, dentry);
	return new_node;
}

static void insert_non_full(fnode *root, file_info *finfo, os_dentry *dentry)
{
	uint32 i = root->head.num;
	//如果是叶子节点 
	if (root->head.leaf == 1)
	{
		//找到插入的位置
		for (; i > 0 && os_str_cmp(root->finfo[i - 1].name, finfo->name) > 0; i--)
		{
			os_mem_cpy(&root->finfo[i], &root->finfo[i - 1], sizeof(file_info));
			dentry->move_callback(root->head.node_id, i, root->finfo[i - 1].cluster_id, dentry->cluster->dev_id);
		}
		os_mem_cpy(&root->finfo[i], finfo, sizeof(file_info));
		root->head.num++; 
		add_flush(root, dentry);
	}
	else
	{
		fnode *new_node = NULL;
		fnode *split_node = NULL;
		//找到插入的位置
		while (i > 0 && os_str_cmp(root->finfo[i - 1].name, finfo->name) > 0)
			i--;

		new_node = fnode_load(root->head.pointers[i], dentry);
		if (new_node->head.num == FS_MAX_KEY_NUM)
		{
			//如果孩子满  
			split_node = split_child(root, i, new_node, dentry);

			split_node->head.next = new_node->head.next;
			new_node->head.next = split_node->head.node_id;
			add_flush(new_node, dentry);
			if (os_str_cmp(root->finfo[i].name, finfo->name) < 0)
			{
				new_node = split_node;
			}
		}
		insert_non_full(new_node, finfo, dentry);
		if (split_node != NULL)
		{
			fnode_flush(split_node, dentry);
			os_free(split_node);
		}
		if (new_node != split_node)
		{
			fnode_free(new_node, dentry);
		}
	}
}

//插入新的节点
fnode *insert_to_btree(fnode *root, file_info *finfo, os_dentry *dentry)
{
	//如果树是空树，创建一棵树 
	if (root == NULL)
	{
		root = make_leaf(dentry);
		root->head.num = 1;
		os_mem_cpy(root->finfo, finfo, sizeof(file_info));
		fnode_flush(root, dentry);
	}
	else // 如果树不为空  
	{
		//如果当前节点满，则分裂
		if (root->head.num == FS_MAX_KEY_NUM)
		{
			fnode *new_node;
			fnode *split_node;
			//生成新的节点
			new_node = make_node(dentry);
			//把root作为新节点的孩子  
			new_node->head.pointers[0] = root->head.node_id;

			//分裂新节点的孩子节点
			split_node = split_child(new_node, 0, root, dentry);

			//把两个孩子连接起来
			root->head.next = split_node->head.node_id;

			//决定插入到哪个孩子中
			if (os_str_cmp(new_node->finfo[0].name, finfo->name) < 0)
			{
				insert_non_full(split_node, finfo, dentry);
			}
			else
			{
				insert_non_full(root, finfo, dentry);
			}
			fnode_flush(split_node, dentry);
			fnode_flush(new_node, dentry);
			os_free(split_node);
			add_flush(root, dentry);
			//改变root
			root = new_node;
		}
		else
		{
			insert_non_full(root, finfo, dentry);
		}

	}
	return root;
}

//融合两个孩子节点
static void merge(fnode *root, fnode *child, fnode *sibling, uint32 idx, os_dentry *dentry)
{
	uint32 i = 0;
	//拷贝内容
	os_mem_cpy(&child->finfo[FS_MAX_KEY_NUM / 2 - 1], &root->finfo[idx], sizeof(file_info));
	dentry->move_callback(child->head.node_id, FS_MAX_KEY_NUM / 2 - 1, root->finfo[idx].cluster_id, dentry->cluster->dev_id);
	for (i = 0; i < sibling->head.num; ++i)
	{
		os_mem_cpy(&child->finfo[i + FS_MAX_KEY_NUM / 2], &sibling->finfo[i], sizeof(file_info));
		dentry->move_callback(child->head.node_id, i + FS_MAX_KEY_NUM / 2, sibling->finfo[i].cluster_id, dentry->cluster->dev_id);
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
		dentry->move_callback(root->head.node_id, i - 1, root->finfo[i].cluster_id, dentry->cluster->dev_id);
	}

	//移动父节点的指针
	for (i = idx + 2; i <= root->head.num; ++i)
		root->head.pointers[i - 1] = root->head.pointers[i];

	//更新父节点和孩子节点计数  
	child->head.num += sibling->head.num + 1;
	root->head.num--;
	child->head.next = sibling->head.next;
	//释放簇 
	cluster_free(sibling->head.node_id, dentry->cluster);
	add_flush(root, dentry);
	add_flush(child, dentry);
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
fnode *find_from_tree(fnode *root, uint32 *index, const char *name, os_dentry *dentry)
{
	fnode *cur = root;
	for(; cur != NULL;)
	{
		for (*index = 0; *index < cur->head.num; (*index)++)
		{
			int8 cmp = os_str_cmp(cur->finfo[*index].name, name);
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
				fnode_free(cur, dentry);
			}
			cur = fnode_load(id, dentry);
		}
		else
		{
			break;
		}

	}
	if (cur != root)
	{
		fnode_free(cur, dentry);
	}
	return NULL;
}

//判断文件是否存在
uint32 finfo_is_exist(fnode *root, const char *name, os_dentry *dentry)
{
	uint32 index;
	fnode *cur = root;
	for (; cur != NULL;)
	{
		for (index = 0; index < cur->head.num; (index)++)
		{
			int8 cmp = os_str_cmp(cur->finfo[index].name, name);
			if (0 == cmp)
			{
				if (cur != root)
				{
					fnode_free(cur, dentry);
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
				fnode_free(cur, dentry);
			}
			cur = fnode_load(id, dentry);
		}
		else
		{
			break;
		}

	}
	if (cur != NULL && cur != root)
	{
		fnode_free(cur, dentry);
	}
	return 0;
}

//遍历文件
uint32 query_finfo(file_info *finfo, uint32 *id, fnode **parent, fnode **cur, uint32 *index, os_dentry *dentry)
{
	if (NULL == *parent)
	{
		*parent = fnode_load(*id, dentry);
		*cur = fnode_load(*id, dentry);
	}
	if (*cur != NULL)
	{
		if (*index == (*cur)->head.num)
		{
			*index = 0;
			*id = (*cur)->head.next;
			if (*id != 0)
			{
				fnode_free(*cur, dentry);
				*cur = fnode_load(*id, dentry);
			}
			else
			{
;				*id = (*parent)->head.pointers[0];
				if (*id != 0)
				{
					fnode_free(*parent, dentry);
					fnode_free(*cur, dentry);
					*parent = fnode_load(*id, dentry);
					*cur = fnode_load(*id, dentry);
				}
				else
				{
					fnode_free(*parent, dentry);
					fnode_free(*cur, dentry);
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
static void get_pred(file_info *finfo, fnode *root, os_dentry *dentry)
{ 
	fnode *cur = root;
	uint32 id = cur->head.pointers[cur->head.num];
	while (!cur->head.leaf)
	{
		if (cur != root)
		{
			fnode_free(cur, dentry);
		}
		cur = fnode_load(id, dentry);
		id = cur->head.pointers[cur->head.num];
	}
	os_mem_cpy(finfo, &cur->finfo[cur->head.num - 1], sizeof(file_info));
	if (cur != root)
	{
		fnode_free(cur, dentry);
	}
}
//找到右边最小的key
static void get_succ(file_info *finfo, fnode *root, os_dentry *dentry)
{
	fnode *cur = root;
	uint32 id = cur->head.pointers[0];
	while (!cur->head.leaf)
	{
		if (cur != root)
		{
			fnode_free(cur, dentry);
		}
		cur = fnode_load(id, dentry);
		id = cur->head.pointers[0];
	}
	os_mem_cpy(finfo, &cur->finfo[0], sizeof(file_info));
	if (cur != root)
	{
		fnode_free(cur, dentry);
	}
}

//从叶子节点中删除
static void remove_from_leaf(fnode *root, uint32 idx, os_dentry *dentry)
{ 
	uint32 i;
	for (i = idx + 1; i < root->head.num; ++i)
	{
		os_mem_cpy(&root->finfo[i - 1], &root->finfo[i], sizeof(file_info));
		dentry->move_callback(root->head.node_id, i - 1, root->finfo[i].cluster_id, dentry->cluster->dev_id);
	}
	root->head.num--;
	add_flush(root, dentry);
	return;
}

//从非叶子节点中删除 
static void remove_from_non_leaf(fnode *root, uint32 idx, os_dentry *dentry)
{
	fnode *node;
	file_info *temp = (file_info *)os_malloc(sizeof(file_info));
	os_mem_cpy(temp, &root->finfo[idx], sizeof(file_info));

	node = fnode_load(root->head.pointers[idx], dentry);
	if (node->head.num >= FS_MAX_KEY_NUM / 2)
	{
		file_info *pred = (file_info *)os_malloc(sizeof(file_info));
		get_pred(pred, node, dentry);
		os_mem_cpy(&root->finfo[idx], pred, sizeof(file_info));
		dentry->move_callback(root->head.node_id, idx, pred->cluster_id, dentry->cluster->dev_id);
		remove_from_btree(node, pred->name, dentry);
		os_free(pred);
	}
	else
	{
		fnode *nnode = fnode_load(root->head.pointers[idx + 1], dentry);
		if (nnode->head.num >= FS_MAX_KEY_NUM / 2)
		{
			file_info *succ = (file_info *)os_malloc(sizeof(file_info));
			get_succ(succ, nnode, dentry);
			os_mem_cpy(&root->finfo[idx], succ, sizeof(file_info));
			dentry->move_callback(root->head.node_id, idx, succ->cluster_id, dentry->cluster->dev_id);
			remove_from_btree(nnode, succ->name, dentry);
			os_free(succ);
		}
		else
		{
			merge(root, node, nnode, idx, dentry);
			remove_from_btree(node, temp->name, dentry);
		}
		fnode_free(nnode, dentry);
	}
	fnode_free(node, dentry);
	os_free(temp);
	add_flush(root, dentry);
}

//从前面的兄弟借值
static void borrow_from_prev(fnode *root, fnode *child, fnode *sibling, uint32 idx, os_dentry *dentry)
{
	uint32 i;
	//移动内容
	for (i = child->head.num; i > 0; --i)
	{
		os_mem_cpy(&child->finfo[i], &child->finfo[i - 1], sizeof(file_info));
		dentry->move_callback(child->head.node_id, i, child->finfo[i - 1].cluster_id, dentry->cluster->dev_id);
	}

	//移动指针
	if (!child->head.leaf)
	{
		for (i = child->head.num + 1; i > 0; --i)
			child->head.pointers[i] = child->head.pointers[i - 1];
	}

	os_mem_cpy(&child->finfo[0], &root->finfo[idx - 1], sizeof(file_info));
	dentry->move_callback(child->head.node_id, 0, root->finfo[idx - 1].cluster_id, dentry->cluster->dev_id);
	if (!root->head.leaf)
		child->head.pointers[0] = sibling->head.pointers[sibling->head.num];

	os_mem_cpy(&root->finfo[idx - 1], &sibling->finfo[sibling->head.num - 1], sizeof(file_info));
	dentry->move_callback(root->head.node_id, idx - 1, sibling->finfo[sibling->head.num - 1].cluster_id, dentry->cluster->dev_id);

	child->head.num += 1;
	sibling->head.num -= 1;
	add_flush(root, dentry);
	add_flush(child, dentry);
	add_flush(sibling, dentry);
}

//从后面的兄弟借值
static void borrow_from_next(fnode *root, fnode *child, fnode *sibling, uint32 idx, os_dentry *dentry)
{
	uint32 i;
	os_mem_cpy(&child->finfo[child->head.num], &root->finfo[idx], sizeof(file_info));
	dentry->move_callback(child->head.node_id, child->head.num, root->finfo[idx].cluster_id, dentry->cluster->dev_id);

	if (!child->head.leaf)
		child->head.pointers[child->head.num + 1] = sibling->head.pointers[0];

	os_mem_cpy(&root->finfo[idx], &sibling->finfo[0], sizeof(file_info));
	dentry->move_callback(root->head.node_id, idx, sibling->finfo[0].cluster_id, dentry->cluster->dev_id);

	for (i = 1; i < sibling->head.num; ++i)
	{
		os_mem_cpy(&sibling->finfo[i - 1], &sibling->finfo[i], sizeof(file_info));
		dentry->move_callback(sibling->head.node_id, i - 1, sibling->finfo[i].cluster_id, dentry->cluster->dev_id);
	}

	if (!sibling->head.leaf)
	{
		for (i = 1; i <= sibling->head.num; ++i)
			sibling->head.pointers[i - 1] = sibling->head.pointers[i];
	}

	child->head.num += 1;
	sibling->head.num -= 1;
	add_flush(root, dentry);
	add_flush(child, dentry);
	add_flush(sibling, dentry);
}
//值不足要填充足够的值
static void fill(fnode *root, fnode *child, uint32 idx, os_dentry *dentry)
{
	fnode *left = NULL;
	if (idx != 0)
	{
		left = fnode_load(root->head.pointers[idx - 1], dentry);
		if (left->head.num >= FS_MAX_KEY_NUM / 2)
		{
			borrow_from_prev(root, child, left, idx, dentry);
			return;
		}

	}

	if (idx != root->head.num)
	{
		fnode *right;
		right = fnode_load(root->head.pointers[idx + 1], dentry);
		if (right->head.num >= FS_MAX_KEY_NUM / 2)
		{
			borrow_from_next(root, child, right, idx, dentry);
		}
		else
		{
			merge(root, child, right, idx, dentry);
		}
	}	
	else
	{
		if (NULL == left)
		{
			left = fnode_load(root->head.pointers[idx - 1], dentry);
		}
		merge(root, left, child, idx - 1, dentry);
	}
}

fnode *remove_from_btree(fnode *root, const char *name, os_dentry *dentry)
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
				remove_from_leaf(root, idx, dentry);
			else
				remove_from_non_leaf(root, idx, dentry);
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

			child = fnode_load(root->head.pointers[idx], dentry);
			if (child->head.num < FS_MAX_KEY_NUM / 2)
			{
				fill(root, child, idx, dentry);
			}

			if (flag && idx > root->head.num)
			{
				fnode *sibling = fnode_load(root->head.pointers[idx - 1], dentry);
				remove_from_btree(sibling, name, dentry);
				fnode_free(sibling, dentry);
			}
			else
				remove_from_btree(child, name, dentry);
			fnode_free(child, dentry);
		}
	}
	//如果root没有值，删除根节点，他的孩子作为根节点
	if (root->head.num == 0)
	{
		fnode *tmp = root;
		if (root->head.leaf)
			root = NULL;
		else
			root = fnode_load(root->head.pointers[0], dentry);
		cluster_free(tmp->head.node_id, dentry->cluster);
		fnode_free_f(tmp, dentry);
	}
	return root;
}

void insert_to_fnodes(fnode *node, os_dentry *dentry)
{
	if (node != NULL)
	{
		fnode_h handle;
		handle.flag = 0;
		handle.count = 1;
		handle.node = node;
		os_map_insert(&dentry->fnodes, &node->head.node_id, &handle);
	}
}

void add_flush(fnode *node, os_dentry *dentry)
{
	if (node != NULL)
	{
		os_map_iterator *itr = os_map_find(&dentry->fnodes, &node->head.node_id);
		if (itr != NULL)
		{
			fnode_h *handle = (fnode_h *)os_map_second(&dentry->fnodes, itr);
			handle->flag = 1;
		}
	}
}