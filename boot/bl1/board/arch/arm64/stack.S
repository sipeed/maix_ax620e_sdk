/*
 * arch/aarch64/stack.S - stack handling
 *
 * Copyright (C) 2015 ARM Limited. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE.txt file.
 */

 #define STACK_SIZE 4096
 #define CPU_IDS		(0)

	.globl setup_stack
	.globl stack_top
	.globl stack_bottom

	.text
	/*
	 * Setup initial stack pointer
	 * x0: logical CPU ID
	 * Clobbers x1 and x2
	 */
setup_stack:
	mov	w1, #STACK_SIZE				// #define STACK_SIZE 1024, w1 = 0x400
	ldr	x2, =stack_top				// which defined in makefile or linker scripts
	umsubl	x0, w0, w1, x2				// sp = st_base - cpu * st_size; Unsigned Multiply-Subtract Long: x0 = x2 - w0 * w1; (x2=0x3000; w0=0/1/2/3)


	mov	sp, x0					//sp
	ret

	.section .stack
	.align 4

stack_bottom:
	.irp cpu, CPU_IDS				// .irp <parameter> {,val_1} {,val_2}...{,val_n} (.irp and .endr) is loop statement
	.space STACK_SIZE
	.endr						// match with .irp
stack_top:						// stack_top = .stack + 0x400; 0x3000 = 0x2800 + 0x400*2
