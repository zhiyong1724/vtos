.global os_cpu_sr_save
os_cpu_sr_save:            //关中断并保存cpsr
	mrs r0, cpsr
	mov r1, r0
	orr r1, r1, #0x80
	msr cpsr, r1
	mov pc, lr

.global os_cpu_sr_restore
os_cpu_sr_restore:         //恢复cpsr
	msr cpsr, r0
	mov pc, lr

.global os_ctx_sw
os_ctx_sw:                //任务级切换，触发软中断即可
	push {r0, lr}
	mov r0, #0
	swi
	pop {lr}
	mov pc, lr

.global os_int_ctx_sw
os_int_ctx_sw:           //中断级切换，跟任务级切换一样
	push {r0, lr}
	mov r0, #0
	swi
	pop {lr}
	mov pc, lr

.global swi_handler
swi_handler:
	cmp r0, #0
	beq sys_handler
	b sys_call

sys_handler:
	pop {r0}
	push {r0-r12, lr}
	ldr r0, =_runnin_task          //获得_runnin_task指针地址
	ldr r1, =_next_task            //获得_next_task指针地址
	ldr r2, [r0]                   //获得_runnin_task地址
	str sp, [r2]                   //保存sp
	ldr r2, [r1]                   //获得_next_task地址
	str r2, [r0]                   //_runnin_task = _next_task
	ldr sp, [r2]                   //sp等于_next_task地址中的值
	mrs r0, cpsr
	bic r0, r0, #0x80
	msr cpsr, r0
	pop {r0-r12, lr, pc}           //直接返回任务

.global os_task_start
os_task_start:
	mrs r0, cpsr                   //开中断
	bic r0, r0, #0x80
	msr cpsr, r0

	ldr r0, =_runnin_task          //获得_runnin_task指针地址
	ldr r1, =_next_task            //获得_next_task指针地址
	ldr r2, [r1]                   //获得_next_task地址
	str r2, [r0]                   //_runnin_task = _next_task
	ldr sp, [r2]                   //sp = _runnin_task->stack
	pop {r0-r12, lr, pc}           //开启第一个任务,这个任务是系统空闲任务
loop:
	b loop

sys_call:
	push {r1}
	ldr r1, =_sys_table
	add r0, r0, r1
	ldr r0, [r0]
	ldr r1, =_call_addr
	str r0, [r1]
	pop {r1}
	pop {r0}
	ldr pc, _call_addr

_call_addr:
	.word 0x00000000


