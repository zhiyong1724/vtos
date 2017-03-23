#include "vtos.h"
const void *_sys_table[] = 
{
	NULL,                                                       //0，保留
	os_sys_tick,                                                //1
	os_sys_init,                                                //2
	os_sys_start,                                               //3
	os_version,                                                 //4
	os_kmalloc,                                                 //5
	os_kfree,                                                   //6
	os_total_mem_size,                                          //7
	os_used_mem_size,                                           //8
	os_kthread_create,                                          //9
	os_kthread_createEX,                                        //10
	os_set_prio,                                                //11
	os_total_thread_count,                                      //12
	os_activity_thread_count,                                   //13
	os_sleep,                                                   //14
	os_sem_create,                                              //15
	os_sem_pend,                                                //16
	os_sem_post,                                                //17
	os_q_create,                                                //18
	os_q_reset,                                                 //19
	os_q_pend,                                                  //20
	os_q_post                                                   //21
};
