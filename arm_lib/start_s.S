.text
.global _start
//初始化异常向量表
_start:
	b reset
	ldr pc, _undifined_instruction
	ldr pc, _software_interrupt
	ldr pc, _prefetch_abort
	ldr pc, _data_abort
	ldr pc, _not_used
	ldr pc, _irq
	ldr pc, _fiq
	

_undifined_instruction: .word undifined_instruction
_software_interrupt: .word software_interrupt
_prefetch_abort: .word prefetch_abort
_data_abort: .word data_abort
_not_used: .word not_used
_irq: .word irq
_fiq: .word reset

undifined_instruction:
	nop

software_interrupt:
	b swi_handler
	
prefetch_abort:
	nop
	
data_abort:
	nop

not_used:
	nop
irq:
	ldr sp, =0x34000000
	push {r0-r1}
	sub r0, lr, #4

	mrs r1, spsr
	orr r1, #0x80
	msr cpsr, r1

	push {r0}
	push {lr}
	push {r2}

	mrs r1, cpsr
	bic r1, r1, #0x1f
	orr r1, r1, #0x12
	msr cpsr, r1
	pop {r0-r1}

	mrs r2, cpsr
	bic r2, r2, #0x1f
	orr r2, r2, #0x13
	msr cpsr, r2
	pop {r2}

	push {r0-r12}
	bl irq_handler
	mrs r0, cpsr
	bic r0, r0, #0x80
	msr cpsr, r0
	pop {r0-r12, lr, pc}
fiq:	
	nop

reset:
	bl set_svc
	bl disable_watchdog
	bl disable_interrupt
	bl disable_mmu
	bl init_clock
	bl init_sdram
	bl copy_to_ram
	bl init_stack
	bl clear_bss
	bl load_code
	ldr pc, =main
//设置svc模式
set_svc:
	mrs r0, cpsr
	bic r0, r0,#0xff
	orr r0, r0,#0xd3
	msr cpsr, r0
	mov pc, lr
//关闭看门狗
#define pWTCON 0x53000000
disable_watchdog:
	ldr r0, =pWTCON
	mov r1, #0x0
	str r1, [r0]
	mov pc, lr
//关闭中断
disable_interrupt:
	mvn r1, #0x0
	ldr r0, =0x4a000008
	str r1, [r0]
	mov pc, lr
//关闭mmu
disable_mmu:
	mcr p15,0,r0,c7,c7,0
	mrc p15,0,r0,c1,c0,0
	bic r0, r0, #0x00000007
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
//时钟初始化
#define CLKDIVN 0x4c000014
#define MPLLCON 0x4c000004
#define MPLL_405MHZ ((127<<12)|(2<<4)|(1<<0))

init_clock:
	ldr r0, =CLKDIVN
	mov r1, #0x5
	str r1, [r0]
	
	mcr p15,0,r0,c1,c0,0
	orr r0,r0,#0xc0000000
	mcr p15,0,r0,c1,c0,0
	
	ldr r0, =MPLLCON
	ldr r1, =MPLL_405MHZ
	str r1, [r0]
	mov pc, lr
//内存初始化
#define mem_contrl 0x48000000
init_sdram:
	ldr r0, =mem_contrl 
	add r3, r0, #4*13
	adrl r1, mem_data

0:
	ldr r2, [r1], #4
	str r2, [r0], #4
	cmp r0, r3
	bne 0b
	mov pc, lr
//把代码从sram拷贝到sdram
copy_to_ram:
	ldr r0, =0x0
	ldr r1, =0x30000000
	add r3, r0, #1024*4

copy_loop:
	ldr r2, [r0]
	str r2, [r1]
	add r0, #4
	add r1, #4
	cmp r0, r3
	bne copy_loop
	mov pc, lr
//初始化栈
init_stack:
	ldr sp, =0x33fff000
	mov pc ,lr
//清空bss段
.globl _bss_start
_bss_start:
	.word bss_start
.globl _bss_end
_bss_end:
	.word bss_end
.globl _heap_start
_heap_start:
	.word heap_start
clear_bss:
	ldr	r0, _bss_start
	ldr	r1, _bss_end
	mov 	r2, #0x00000000

clbss_l:
	str	r2, [r0]
	add	r0, r0, #4
	cmp	r0, r1
	ble	clbss_l
	mov pc ,lr

.global to_swi
to_swi:
	push {lr}
	swi
	pop {lr}
	mov pc, lr

.global raise
raise:
	mov pc, lr

mem_data:
	.word 0x22000000
	.word 0x00000700
	.word 0x00000700
	.word 0x00000700
	.word 0x00000700
	.word 0x00000700
	.word 0x00000700
	.word 0x00018001
	.word 0x00018001
	.word 0x008c04f5
	.word 0x000000b1
	.word 0x00000030
	.word 0x00000030

