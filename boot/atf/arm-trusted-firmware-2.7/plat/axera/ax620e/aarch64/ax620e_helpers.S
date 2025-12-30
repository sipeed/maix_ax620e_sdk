/*
 * Copyright (c) 2017-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>


	.text

	.globl	plat_my_core_pos

func plat_my_core_pos
	mrs	x0, mpidr_el1
	mov x1, #MPIDR_CPU_MASK
	and	x0, x1, x0
	ret
endfunc plat_my_core_pos

	.globl	ret_to_addr

func ret_to_addr
	lsl x0, x0, #32
	orr x0, x0, x1
	br x0
endfunc ret_to_addr

	.global cntkctl_enable
func cntkctl_enable
	mrs x0, cntkctl_el1
	orr x0, x0, (1 << 2)
	msr cntkctl_el1, x0
endfunc cntkctl_enable

	.global cntkctl_disable
func cntkctl_disable
	mrs x0, cntkctl_el1
	bic x0, x0, (1 << 2)
	msr cntkctl_el1, x0
endfunc cntkctl_disable




