.global os_cpu_sr_off
os_cpu_sr_off:            //关中断并保存cpsr
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
	push {lr}
	swi 0
	pop {lr}
	mov pc, lr

.global swi_handler
swi_handler:
	push {r0-r12, lr}
	ldr r0, =_running_task          //获得_running_task指针地址
	ldr r1, =_next_task            //获得_next_task指针地址
	ldr r2, [r0]                   //获得_running_task地址
	str sp, [r2]                   //保存sp
	ldr r2, [r1]                   //获得_next_task地址
	str r2, [r0]                   //_running_task = _next_task
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

	ldr r0, =_running_task          //获得_running_task指针地址
	ldr r1, =_next_task            //获得_next_task指针地址
	ldr r2, [r1]                   //获得_next_task地址
	str r2, [r0]                   //_running_task = _next_task
	ldr sp, [r2]                   //sp = _running_task->stack
	pop {r0-r12, lr, pc}           //开启第一个任务,这个任务是系统空闲任务
loop:
	b loop



