#include "fs/os_fs.h"
#include "fs/os_dentry.h"
#include "lib/os_string.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib/os_string.h"
#include "vtos.h"
static void *root = NULL;
static fnode *load(uint32 id)
{
	fnode *node = (fnode *)malloc(sizeof(fnode));
	if (node != NULL)
	{
		os_disk_read(id, node);
		if (!is_little_endian())
		{
			uint32 i = 0;
			node->head.node_id = convert_endian(node->head.node_id);
			node->head.num = convert_endian(node->head.num);
			node->head.leaf = convert_endian(node->head.leaf);
			for (; i < FS_MAX_KEY_NUM + 1; i++)
			{
				node->head.pointers[i] = convert_endian(node->head.pointers[i]);
			}
		}

	}
	return node;
}

static void p(fnode *node)
{
	uint32 i = 0;
	for (; i < node->head.num; i++)
	{
		printf(node->finfo[i].name);
		printf(" ");
	}
}

static void pp(fnode *node, uint32 *num, fnode **pi)
{
	uint32 i = 0;
	if (num != NULL)
		*num = 0;
	for (; i <= node->head.num; i++)
	{
		if (node->head.pointers[i] != 0)
		{
			fnode *inode = NULL;
			inode = load(node->head.pointers[i]);
			p(inode);
			printf("|");
			if (pi != NULL)
				pi[i] = inode;
			else
				free(inode);
			if (num != NULL)
			 (*num)++;
		}
	}
}

static void ppp(fnode *node)
{
	if (node != NULL)
	{
		uint32 i = 0;
		uint32 num;
		fnode *pi[32];
		p(node);
		printf("\n");
		pp(node, &num, pi);
		printf("\n");
		for (; i < num; i++)
		{
			if (pi[i]->head.leaf == 0)
			{
				pp(pi[i], NULL, NULL);
				printf(",");
			}
			free(pi[i]);
		}
	}
}

void add()
{
	file_info file;
	char buff[16] = "";
	printf("输入q!返回上一层。\n");
	ppp(root);
	while (1)
	{
		scanf_s("%s", buff, 16);
		if (os_str_cmp(buff, "q!") == 0)
			break;
		os_str_cpy(file.name, buff, 16);
		root = insert_to_btree(root, &file);
		system("cls");
		ppp(root);
	}
}

void rm()
{
	file_info file;
	char buff[16] = "";
	printf("输入q!返回上一层。\n");
	ppp(root);
	while (1)
	{
		scanf_s("%s", buff, 16);
		if (os_str_cmp(buff, "q!") == 0)
			break;
		os_str_cpy(file.name, buff, 16);
		root = remove_from_btree(root, &file);
		system("cls");
		ppp(root);
	}
}

void test()
{
	char buff[16] = "";
	while (1)
	{
		printf("tnode = %d\nfile_info = %d\nfnode = %d\n", sizeof(tnode), sizeof(file_info), sizeof(fnode));
		printf("请输入i进行插入操作，输入d进行删除操作,输入q!退出。\n");
		scanf_s("%s", buff, 16);
		if (os_str_cmp(buff, "q!") == 0)
			break;
		else if (os_str_cmp(buff, "i") == 0)
		{
			add();
		}
		else if (os_str_cmp(buff, "d") == 0)
		{
			rm();
		}
		system("cls");
	}
}



