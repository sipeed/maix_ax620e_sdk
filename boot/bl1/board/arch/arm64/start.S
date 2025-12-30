#include "common.S"
	.text

	.globl _start
_start:					// the first line code

#ifndef SUPPORT_ATF

	mrs x0, scr_el3
	orr x0, x0, #0xf		/* SCR_EL3.NS|IRQ|FIQ|EA */
	msr scr_el3, x0
	msr cptr_el3, xzr		/* Enable FP/SIMD */
	ldr x0, =COUNTER_FREQUENCY
	msr cntfrq_el0, x0		/* Initialize CNTFRQ */

	mrs x0, S3_1_c15_c2_1 		/* cpuactlr_el1 */
	orr x0, x0, #0x40
	msr S3_1_c15_c2_1, x0

	mrs x0, mpidr_el1		// MPIDR_EL1: dedicated register, Multiprocessor Affinity Register for Process ID,(MPIDR_EL1)=0x80000000
	and x0, x0, #0x0F		// select CPU ID number

	cmp x0,#0x1
	b.eq second_core

	mov x0, #0x30			// RES1
	orr x0, x0, #(1 << 0)		// Non-secure EL1
	orr x0, x0, #(1 << 8)		// HVC enable
	orr x0, x0, #(1 << 10)		// 64-bit EL2
	msr scr_el3, x0
	msr cptr_el3, xzr			// Disable copro. traps to EL3

	mov x0, #0x0
	bl  setup_stack			// setup_stack in stack.S

	bl clear_bss_section

	bl gic400_secure_init

	bl main

	/*
	* Prepare the switch to the EL2_SP1 mode from EL3
	*/
	ldr x0, =SCTLR_EL2_RESET
	msr sctlr_el2, x0
	ldr x0, =main_start_el2	// Return after mode switch
	mov x1, #SPSR_KERNEL
	drop_el x1, x0

	/*
	* Prepare the switch to El1 mode from EL2
	*/
main_start_el2:
	mrs x0, cnthctl_el2
	orr x0, x0, #(CNTHCTL_EL2_EL1PCEN_EN | CNTHCTL_EL2_EL1PCTEN_EN)
	msr cnthctl_el2, x0		// Enable EL1 access to timers
	msr cntvoff_el2, xzr

	/* Initilize MPID/MPIDR registers */
	mrs x0, midr_el1
	msr vpidr_el2,x0
	mrs	x0, mpidr_el1
	msr	vmpidr_el2, x0

	/* Disable coprocessor traps */
	mov	x0, #CPTR_EL2_RES1
	msr	cptr_el2, x0	/* Disable coprocessor traps to EL2 */
	msr	hstr_el2, xzr	/* Disable coprocessor traps to EL2 */
	mov	x0, #CPACR_EL1_FPEN_EN
	msr	cpacr_el1, x0	/* Enable FP/SIMD at EL1 */

	/* SCTLR_EL1 initialization
	 *
	 * setting RES1 bits (29,28,23,22,20,11) to 1
	 * and RES0 bits (31,30,27,21,17,13,10,6) +
	 * UCI,EE,EOE,WXN,nTWE,nTWI,UCT,DZE,I,UMA,SED,ITD,
	 * CP15BEN,SA0,SA,C,A,M to 0
	 */
	ldr	x0, =(SCTLR_EL1_RES1 | SCTLR_EL1_UCI_DIS |\
			SCTLR_EL1_EE_LE | SCTLR_EL1_WXN_DIS |\
			SCTLR_EL1_NTWE_DIS | SCTLR_EL1_NTWI_DIS |\
			SCTLR_EL1_UCT_DIS | SCTLR_EL1_DZE_DIS |\
			SCTLR_EL1_ICACHE_DIS | SCTLR_EL1_UMA_DIS |\
			SCTLR_EL1_SED_EN | SCTLR_EL1_ITD_EN |\
			SCTLR_EL1_CP15BEN_DIS | SCTLR_EL1_SA0_DIS |\
			SCTLR_EL1_SA_DIS | SCTLR_EL1_DCACHE_DIS |\
			SCTLR_EL1_ALIGN_DIS | SCTLR_EL1_MMU_DIS)
	msr	sctlr_el1, x0
	mov	x0, sp
	msr	sp_el1, x0	/* Migrate SP */
	mrs	x0, vbar_el2
	msr	vbar_el1, x0	/* Migrate VBAR */

	/* Initialize HCR_EL2 */
	ldr	x0, =(HCR_EL2_RW_AARCH64 | HCR_EL2_HCD_DIS)
	msr	hcr_el2, x0

	/* Return to the EL1_SP1 mode from EL2 */
	ldr	x0, =(SPSR_EL_DEBUG_MASK | SPSR_EL_SERR_MASK |\
			SPSR_EL_IRQ_MASK | SPSR_EL_FIQ_MASK |\
			SPSR_EL_M_AARCH64 | SPSR_EL_M_EL1H)
	msr spsr_el2, x0
	ldr x0, =main_start_el1
	msr elr_el2,x0
	eret

main_start_el1:
	mov x0, xzr
	mov x1, xzr
	mov x2, xzr
	mov x3, xzr
	mov x4, x23
	mov x0, x24		/* get dtb addr before jump to kernel when not use uboot */
	br x4			/* jump to kernel or uboot */

second_core:

	mov x0, #0x30			// RES1
	orr x0, x0, #(1 << 0)		// Non-secure EL1
	orr x0, x0, #(1 << 8)		// HVC enable
	orr x0, x0, #(1 << 10)		// 64-bit EL2
	msr scr_el3, x0
	msr cptr_el3, xzr		// Disable copro. traps to EL3
	bl gic400_secure_init

	/*
	* Prepare the switch to the EL2_SP1 mode from EL3
	*/
	ldr x0, =SCTLR_EL2_RESET
	msr sctlr_el2, x0
	ldr x0, =start_el2 		// Return after mode switch
	mov x1, #SPSR_KERNEL
	drop_el x1, x0

start_el2:
	mrs x0, cnthctl_el2
	orr x0, x0, #(CNTHCTL_EL2_EL1PCEN_EN | CNTHCTL_EL2_EL1PCTEN_EN)
	msr cnthctl_el2, x0
	msr cntvoff_el2, xzr

	mrs x0, midr_el1
	msr vpidr_el2,x0
	mrs	x0, mpidr_el1
	msr	vmpidr_el2, x0

	mov	x0, #CPTR_EL2_RES1
	msr	cptr_el2, x0
	msr	hstr_el2, xzr
	mov	x0, #CPACR_EL1_FPEN_EN
	msr	cpacr_el1, x0

	ldr	x0, =(SCTLR_EL1_RES1 | SCTLR_EL1_UCI_DIS |\
			SCTLR_EL1_EE_LE | SCTLR_EL1_WXN_DIS |\
			SCTLR_EL1_NTWE_DIS | SCTLR_EL1_NTWI_DIS |\
			SCTLR_EL1_UCT_DIS | SCTLR_EL1_DZE_DIS |\
			SCTLR_EL1_ICACHE_DIS | SCTLR_EL1_UMA_DIS |\
			SCTLR_EL1_SED_EN | SCTLR_EL1_ITD_EN |\
			SCTLR_EL1_CP15BEN_DIS | SCTLR_EL1_SA0_DIS |\
			SCTLR_EL1_SA_DIS | SCTLR_EL1_DCACHE_DIS |\
			SCTLR_EL1_ALIGN_DIS | SCTLR_EL1_MMU_DIS)
	msr	sctlr_el1, x0
	mov	x0, sp
	msr	sp_el1, x0
	mrs	x0, vbar_el2
	msr	vbar_el1, x0

	ldr	x0, =(HCR_EL2_RW_AARCH64 | HCR_EL2_HCD_DIS)
	msr	hcr_el2, x0
	ldr	x0, =(SPSR_EL_DEBUG_MASK | SPSR_EL_SERR_MASK |\
			SPSR_EL_IRQ_MASK | SPSR_EL_FIQ_MASK |\
			SPSR_EL_M_AARCH64 | SPSR_EL_M_EL1H)
	msr spsr_el2, x0
	ldr x0, =start_el1
	msr elr_el2,x0
	eret

start_el1:
	mov x0,xzr
	ldr x4, =COMM_SYS_DUMMY_SW1 //COMM_SYS_DUMMY_SW1 bit[7:0] is valid
	str w0, [x4]
	ldr x5, =COMM_SYS_DUMMY_SW0 //COMM_SYS_DUMMY_SW0 bit[31:0]
	str w0, [x5]
2:  wfe					// WFE: wait for event; WFI:wait for interrupt
	ldr x4, =COMM_SYS_DUMMY_SW1 //COMM_SYS_DUMMY_SW1 bit[7:0] is valid
	ldr w4, [x4]
	ldr x5, =COMM_SYS_DUMMY_SW0 //COMM_SYS_DUMMY_SW0 bit[31:0]
	ldr w5, [x5]
	lsl x4, x4, #32		// logical shift left 32 bit
	orr x4, x4, x5		// x4 = x4 | x5
	cbz x4, 2b 		// CBZ:compare and branch zero: conditionally jumps to 2b if x4 is equal to zero.
	br x4 			// BR: branch to the given address

gic400_secure_init:
	/*
	* Check for the primary CPU to avoid a race on the distributor
	* registers.
	*/
	mrs x0, mpidr_el1
	ldr x1, =MPIDR_ID_BITS
	tst x0, x1
	b.ne 1f 		// secondary CPU

	ldr x1, =GICD_BASE
	mov w0, #3 		// EnableGrp0 | EnableGrp1
	str w0, [x1]
1:	ldr x1, =GICD_BASE
	add x1, x1, #0x80 	// GICD_IGROUPR
	mov w0, #~0 		// Grp1 interrupts
	str w0, [x1]
	b.ne 2f 		// Only local interrupts for secondary CPUs

	ldr x2, =GICD_BASE
	add x2, x2, #0x4 	// GICD_TYPER
	ldr w3, [x2]
	ands w3, w3, #0x1f 	// ITLinesNumber
	b.eq 2f
1:	str w0, [x1, #4]!
	subs w3, w3, #1
	b.ne 1b

2:	ldr x1, =GICC_BASE
	mov w0, #3 		// EnableGrp0 | EnableGrp1
	str w0, [x1]

	mov w0, #1 << 7 	// allow NS access to GICC_PMR
	str w0, [x1, #4] 	// GICC_PMR
	ret

	.ltorg
	.org    0x300
#else
	mrs x0, mpidr_el1		// MPIDR_EL1: dedicated register, Multiprocessor Affinity Register for Process ID,(MPIDR_EL1)=0x80000000
	and x0, x0, #0x0F		// select CPU ID number
	bl  setup_stack			// setup_stack in stack.S
	bl clear_bss_section
	ldr	x0, =COUNTER_FREQUENCY
	msr	cntfrq_el0, x0		/* Initialize CNTFRQ */
	bl main
	.ltorg
	.org    0x100
#endif

