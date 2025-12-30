#include <arch.h>
#include <asm_macros.S>
#include <ax620e_common_sys_glb.h>
#include <platform_def.h>
#include <ax620e_def.h>

.macro	ax_dcache_line_size  reg, tmp
	mrs	\tmp, ctr_el0
	ubfx	\tmp, \tmp, #16, #4
	mov	\reg, #4
	lsl	\reg, \reg, \tmp
.endm

.macro ax_do_dcache_maintenance_by_mva op
	/* Exit early if size is zero */
	ax_dcache_line_size x2, x3
	add	x1, x0, x1
	sub	x3, x2, #1
	bic	x0, x0, x3
loop_\op:
	dc	\op, x0
	add	x0, x0, x2
	cmp	x0, x1
	b.lo	loop_\op
	dsb	sy
.endm

.macro ax_second_core_0_do_dcache_maintenance_by_mva op
	/* Exit early if size is zero */
	ax_dcache_line_size x2, x3
	add	x1, x0, x1
	sub	x3, x2, #1
	bic	x0, x0, x3
loop0_\op:
	dc	\op, x0
	add	x0, x0, x2
	cmp	x0, x1
	b.lo	loop0_\op
	dsb	sy
.endm

.macro ax_second_core_1_do_dcache_maintenance_by_mva op
	/* Exit early if size is zero */
	ax_dcache_line_size x2, x3
	add	x1, x0, x1
	sub	x3, x2, #1
	bic	x0, x0, x3
loop1_\op:
	dc	\op, x0
	add	x0, x0, x2
	cmp	x0, x1
	b.lo	loop1_\op
	dsb	sy
.endm

	.global core_wakeup_from_ram
core_wakeup_from_ram:

	/* eic mask clr */
	adr x0, EIC_MASK_EN_CLR_POS
	ldr w0, [x0]
	mov w1, #1
	str w1, [x0]

	mov x1, 0x4e00
	mov sp, x1
	adr x0, ATF_IMG_ADDR_POS0
	ldr w0, [x0]
	adr x1, ATF_IMG_PKG_SIZE_POS0
	ldr w1, [x1]
	ax_do_dcache_maintenance_by_mva ivac
#ifdef SUSPEND_EB
	mov x0, DDR_SYS_WAKEUP_RAM_ADDR
	blr x0
#endif
	adr x0, bl31_warm_entrypoint_pos
	ldr w0,[x0]
	br x0
	//ret

	.global ATF_IMG_ADDR_POS0
ATF_IMG_ADDR_POS0:
	.long ATF_IMG_ADDR

	.global ATF_IMG_PKG_SIZE_POS0
ATF_IMG_PKG_SIZE_POS0:
	.long ATF_IMG_PKG_SIZE

	.global bl31_warm_entrypoint_pos
bl31_warm_entrypoint_pos:
	.long bl31_warm_entrypoint

	.global EIC_MASK_EN_CLR_POS
EIC_MASK_EN_CLR_POS:
	.long EIC_MASK_EN_CLR

	.global core_run_on_ram
core_run_on_ram:

	mrs x0, mpidr_el1
	and x0, x0, #0xff
	cmp x0, #0x0
	b.ne 1f
	adr x0, CA53_CFG_RVBARADDR0_H_POS
	ldr w0, [x0]
	adr x1, CA53_CFG_RVBARADDR0_L_POS
	ldr w1, [x1]
	mov x3, SECOND_CORE_WAKEUP_FROM_RAM_ADDR
	and x3, x3, #0xffffffff
	mov x4, x3, lsr #32	//high bit
	and x5, x3, #0xffffffff  //low bit
	str w5, [x1]
	str w4, [x0]

	adr x0, ATF_IMG_ADDR_POS1
	ldr w0, [x0]
	adr x1, ATF_IMG_PKG_SIZE_POS1
	ldr w1, [x1]
	ax_second_core_0_do_dcache_maintenance_by_mva cvac

	isb
	dsb sy
#ifdef SUSPEND_EB
	wfi
	nop
	nop
	nop
	nop
	nop
	nop
#endif
	b .
1:
	adr x0, CA53_CFG_RVBARADDR1_H_POS
	ldr w0,[x0]
	adr x1, CA53_CFG_RVBARADDR1_L_POS
	ldr w1,[x1]
	mov x3, SECOND_CORE_WAKEUP_FROM_RAM_ADDR
	and x3, x3, #0xffffffff
	mov x4, x3, lsr #32	//high bit
	and x5, x3, #0xffffffff  //low bit
	str w5, [x1]
	str w4, [x0]
	adr x0, ATF_IMG_ADDR_POS1
	ldr w0, [x0]
	adr x1, ATF_IMG_PKG_SIZE_POS1
	ldr w1, [x1]
	ax_second_core_1_do_dcache_maintenance_by_mva cvac
	isb
	dsb sy
#ifdef SUSPEND_EB
	wfi
	nop
	nop
	nop
	nop
	nop
	nop
#endif
	b .

	.global ATF_IMG_ADDR_POS1
ATF_IMG_ADDR_POS1:
	.long ATF_IMG_ADDR

	.global ATF_IMG_PKG_SIZE_POS1
ATF_IMG_PKG_SIZE_POS1:
	.long ATF_IMG_PKG_SIZE

	.global CA53_CFG_RVBARADDR0_H_POS
CA53_CFG_RVBARADDR0_H_POS:
	.long CA53_CFG_RVBARADDR0_H

	.global CA53_CFG_RVBARADDR0_L_POS
CA53_CFG_RVBARADDR0_L_POS:
	.long CA53_CFG_RVBARADDR0_L

	.global CA53_CFG_RVBARADDR1_H_POS
CA53_CFG_RVBARADDR1_H_POS:
	.long CA53_CFG_RVBARADDR1_H

	.global CA53_CFG_RVBARADDR1_L_POS
CA53_CFG_RVBARADDR1_L_POS:
	.long CA53_CFG_RVBARADDR1_L


	.global second_core_wakeup_from_ram
second_core_wakeup_from_ram:
	mrs x0, cntkctl_el1
	bic x0, x0, (1 << 2)
	msr cntkctl_el1, x0
	mrs x0, mpidr_el1
	and x0, x0, #0xff
	cmp x0, #0x0
	b.ne 2f
	adr x4, COMM_SYS_DUMMY_SW3_CORE0_HIGH_POS
	ldr w4,[x4]
	mov x0,#0
	str w0, [x4]
	adr x5, COMM_SYS_DUMMY_SW2_CORE0_LOW_POS
	ldr w5, [x5]
	str w0, [x5]
	b 3f

2:
	adr x4, COMM_SYS_DUMMY_SW1_CORE1_HIGH_POS
	ldr w4, [x4]
	mov x0,#0
	str w0, [x4]
	adr x5, COMM_SYS_DUMMY_SW0_CORE1_LOW_POS
	ldr w5, [x5]
	str w0, [x5]
	b 4f

3:
	wfe
	adr x4, COMM_SYS_DUMMY_SW3_CORE0_HIGH_POS
	ldr w4, [x4]
	ldr w4, [x4]
	adr x5, COMM_SYS_DUMMY_SW2_CORE0_LOW_POS
	ldr w5, [x5]
	ldr w5, [x5]
	lsl x4, x4, #32
	orr x4, x4, x5
	cbz x4, 3b

	adr x5, COMM_SYS_DUMMY_SW3_CORE0_HIGH_POS
	ldr w5,[x5]
	mov x0,#0
	str w0, [x5]
	adr x6, COMM_SYS_DUMMY_SW2_CORE0_LOW_POS
	ldr w6, [x6]
	str w0, [x6]

	mrs x0, cntkctl_el1
	orr x0, x0, (1 << 2)
	msr cntkctl_el1, x0

	br x4

4:
	wfe
	adr x4, COMM_SYS_DUMMY_SW1_CORE1_HIGH_POS
	ldr w4, [x4]
	ldr w4, [x4]
	adr x5, COMM_SYS_DUMMY_SW0_CORE1_LOW_POS
	ldr w5, [x5]
	ldr w5, [x5]
	lsl x4, x4, #32
	orr x4, x4, x5
	cbz x4, 4b

	adr x5, COMM_SYS_DUMMY_SW1_CORE1_HIGH_POS
	ldr w5,[x5]
	mov x0,#0
	str w0, [x5]
	adr x6, COMM_SYS_DUMMY_SW0_CORE1_LOW_POS
	ldr w6, [x6]
	str w0, [x6]

	mrs x0, cntkctl_el1
	orr x0, x0, (1 << 2)
	msr cntkctl_el1, x0

	br x4

	ret

	.global COMM_SYS_DUMMY_SW3_CORE0_HIGH_POS
COMM_SYS_DUMMY_SW3_CORE0_HIGH_POS:
	.long COMM_SYS_DUMMY_SW3_CORE0_HIGH

	.global COMM_SYS_DUMMY_SW2_CORE0_LOW_POS
COMM_SYS_DUMMY_SW2_CORE0_LOW_POS:
	.long COMM_SYS_DUMMY_SW2_CORE0_LOW

	.global COMM_SYS_DUMMY_SW1_CORE1_HIGH_POS
COMM_SYS_DUMMY_SW1_CORE1_HIGH_POS:
	.long COMM_SYS_DUMMY_SW1_CORE1_HIGH

	.global COMM_SYS_DUMMY_SW0_CORE1_LOW_POS
COMM_SYS_DUMMY_SW0_CORE1_LOW_POS:
	.long COMM_SYS_DUMMY_SW0_CORE1_LOW

	.global suspend_on_ram
suspend_on_ram:
#ifdef SUSPEND_EB
	mov x0, DDR_SYS_SLEEP_RAM_ADDR
	blr x0
	mov x0, CPU_SYS_SLEEP_RAM_ADDR
	blr x0
	adr x0, ATF_IMG_ADDR_POS
	ldr w0, [x0]
	adr x1, ATF_IMG_PKG_SIZE_POS
	ldr w1, [x1]
	ax_do_dcache_maintenance_by_mva cvac
	isb
	dsb sy
	wfi
	nop
	nop
	nop
	nop
	nop
	nop
	b .
#endif

	.global ATF_IMG_ADDR_POS
ATF_IMG_ADDR_POS:
	.long ATF_IMG_ADDR

	.global ATF_IMG_PKG_SIZE_POS
ATF_IMG_PKG_SIZE_POS:
	.long ATF_IMG_PKG_SIZE

	.global jump_to_ram
jump_to_ram:
	ldr x1, =0x4ff0
	mov sp, x1
	br x0
