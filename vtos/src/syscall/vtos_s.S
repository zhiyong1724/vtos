_call_addr: .word #0
.global sys_call
sys_call:
	push {r1}
	ldr r1, _sys_table
	add r0, r0, r1
	ldr r1, =_call_addr
	str r0, [r1]
	pop {r1}
	pop {r0}
	ldr pc, _call_addr

os_sys_tick:
	push {r0}
	mov r0, #4 * #1
	swi

os_sys_init:
	push {r0}
	mov r0, #4 * #2
	swi

os_sys_start:
	push {r0}
	mov r0, #4 * #3
	swi

os_version:
	push {r0}
	mov r0, #4 * #4
	swi

os_kmalloc:
	push {r0}
	mov r0, #4 * #5
	swi

os_kfree:
	push {r0}
	mov r0, #4 * #6
	swi

os_total_mem_size:
	push {r0}
	mov r0, #4 * #7
	swi

os_used_mem_size:
	push {r0}
	mov r0, #4 * #8
	swi

os_kthread_create:
	push {r0}
	mov r0, #4 * #9
	swi

os_kthread_createEX:
	push {r0}
	mov r0, #4 * #10
	swi

os_set_prio:
	push {r0}
	mov r0, #4 * #11
	swi

os_total_thread_count:
	push {r0}
	mov r0, #4 * #12
	swi

os_activity_thread_count:
	push {r0}
	mov r0, #4 * #13
	swi

os_sleep:
	push {r0}
	mov r0, #4 * #14
	swi

os_sem_create:
	push {r0}
	mov r0, #4 * #15
	swi

os_sem_pend:
	push {r0}
	mov r0, #4 * #16
	swi

os_sem_post:
	push {r0}
	mov r0, #4 * #17
	swi

os_q_create:
	push {r0}
	mov r0, #4 * #18
	swi

os_q_reset:
	push {r0}
	mov r0, #4 * #19
	swi

os_q_pend:
	push {r0}
	mov r0, #4 * #20
	swi

os_q_post:
	push {r0}
	mov r0, #4 * #21
	swi

