#include "common.S"

	.globl _start
	.section .text
_start:
	source_array:
		.word 0xd5033f9f
		.word 0xd5033fdf
		.word 0xd2800040
		.word 0xd51ec040
		.word 0x580000c0
		.word 0x580000e1
		.word 0xd2800002
		.word 0xf9000041
		.word 0xd5033fdf
		.word 0xd503207f
		.word 0x03000440
		.word 0x00000000
		.word 0xe12fff10
		.word 0x00000000
	.align 6

/*
* .section .text
* _start:
* 	dsb     sy
* 	isb     sy
* 	mov     x0, #0x2        // x0,#2
* 	msr     RMR_EL3,x0
* 	ldr x0, =0x3000440      //start_run addr
* 	ldr x1, =0xe12fff10     //bx r0
* 	mov x2, #0
* 	str x1, [x2]
* 	isb     sy
* 	wfi                    // jump 0x0 addr
* 	.ltorg
* 	.align 6
*/

start_run:
/*
* set smp cluster coherency
*/
	/* Enable the the VFP */
	mrc	p15, 0, r1, c1, c0, 2
	orr	r1, r1, #(0xf << 20)
	mcr	p15, 0, r1, c1, c0, 2
	isb
	fmrx	r1, FPEXC
	orr	r1,r1, #(1<<30)
	fmxr	FPEXC, r1

	mrrc p15, 1, r0, r1, c15 //read CPUECTLR
	orr r0, r0, #0x40
	mcrr p15, 1, r0, r1, c15 //write CPUECTLR


	ldr r0, =COUNTER_FREQUENCY
	MCR p15, 0, r0, c14, c0, 0   /* Initialize CNTFRQ */

/*
* read core id and enter different branches
*/
	mrc p15, 0, r0, c0, c0, 5	// MPIDR
	and r0, r0, #15			// CPU number
	cmp r0, #0
	beq 2f

secondary_cpu:
	//get core id
	mrc p15, 0, r0, c0, c0, 5	// MPIDR
	and r0, r0, #15			// CPU number

	//store -1 to pen_release[cpu_id]
	adr r1, 3f
	ldr r1, [r1]
	lsl r0, r0, #2
	add r1, r1, r0			//get pen_release[cpu_id] addr
	mvn r2, #0
	str r2, [r1]			//store -1 to pen_release[cpu_id]

	//loop dummy register to 0
loop_dummy:
	ldr r0, =COMM_SYS_DUMMY_SW0
	ldr r0, [r0]
	cmp r0, #0
	bne loop_dummy
secondary_holding_pen:
	wfe
	ldr r0, =COMM_SYS_DUMMY_SW0
	ldr r0, [r0]
	cmp r0, #0
	beq secondary_holding_pen
	bx r0				//branch to the given address

2:
	ldr r0, =COUNTER_FREQUENCY
	MCR p15, 0, r0, c14, c0, 0   	/* Initialize CNTFRQ */

	/* enable ISB and DSB access */
	mrc p15, 0, r0, c1, c0, 0 	//Read SCTLR into Rt
	orr r0, r0, #(1 << 5)
	mcr p15, 0, r0, c1, c0, 0 	//Write Rt to SCTLR

	mov r0, #0
	bl setup_stack                 // setup_stack in stack.S
	bl clear_bss_section

	bl main

	.align 2
.global pen_release
pen_release:
	.word 0
	.word 0
3: .long pen_release

	.ltorg
	.org    0x300
