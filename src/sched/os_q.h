#ifndef __OS_Q_H__
#define __OS_Q_H__
#include "sched/os_sem.h"
#include "base/os_deque.h"
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
	typedef struct os_q_t
	{
		tree_node_type_def tree;
		os_sem_t *sem;
		os_deque queue;
		char name[SEM_NAME_SIZE];
	} os_q_t;

	struct os_q
	{
		tree_node_type_def *tree;
	};

	/*********************************************************************************************************************
	* 初始化消息队列控制器
	*********************************************************************************************************************/
	void os_q_init();
	/*********************************************************************************************************************
	* 卸载消息队列控制器
	*********************************************************************************************************************/
	void os_q_uninit();
	/*********************************************************************************************************************
	* 创建一个消息队列
	* unit_size：元素大小
	* name：名称
	* return：返回一个消息队列
	*********************************************************************************************************************/
	os_q_t *os_q_create(os_size_t unit_size, const char *name);
	/*********************************************************************************************************************
	* 重置消息队列，重置后消息队列的内容会丢失
	* p_queue：指向要被重置的消息队列控制器
	*********************************************************************************************************************/
	void os_q_reset(os_q_t *p_queue);
	/*********************************************************************************************************************
	* 获取一个消息，该函数会阻塞线程
	* p_queue：指向要获取消息的消息队列控制器
	* timeout：获取超时时间，单位ms，超时后直接返回NULL，如果是0则表示不设置超时，线程被阻塞直到获取到消息
	* p_status：指向一个空间，函数返回后，会把获取状态存放到这个空间上
	* out：提供消息保存空间
	* return：返回获取到的消息，如果获取超时，则返回NULL
	*********************************************************************************************************************/
	void *os_q_pend(os_q_t *p_queue, os_size_t timeout, os_size_t *p_status, void *p_out);
	/*********************************************************************************************************************
	* 推一条消息进消息队列
	* p_queue：指向消息队列控制器
	* p_msg：要进消息队列的消息指针
	* return：返回消息队列中消息的数量
	*********************************************************************************************************************/
	os_size_t os_q_post(os_q_t *p_queue, void *p_msg);
	/*********************************************************************************************************************
	* 通过名字找到消息队列
	* return：消息队列
	*********************************************************************************************************************/
	os_q_t *os_q_find(const char *name);
	/*********************************************************************************************************************
	* 释放消息队列
	* p_queue：消息队列
	*********************************************************************************************************************/
	void os_q_free(os_q_t *p_queue);
#ifdef __cplusplus
}
#endif // __cplusplus
#endif
