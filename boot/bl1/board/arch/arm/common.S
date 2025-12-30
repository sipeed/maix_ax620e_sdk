/*
 * common.S - common definitions useful for boot code
 *
 * Copyright (C) 2013 ARM Limited. All rights reserved.
 * Copyright (C) 2016 Synopsys, Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE.txt file.
 */
#define COUNTER_FREQUENCY 24000000
#define MPIDR_ID_BITS (0xff00ffffff)	//Multi-Processor Affinity Register

#define COMM_SYS_DUMMY_SW0    (0x02340000 + 0xDC) //second core jump addr [31:0]
#define COMM_SYS_DUMMY_SW1    (0x02340000 + 0xE0) //second core jump addr [39:32]

/*
 *CurrentEL register(32-bit, read only): current exception level.
 * bit[31:4], bit[1:0] Reserved.
 * EL bit[3:2]:
 * 00 for EL0
 * 01 for EL1
 * 10 for EL2
 * 11 for EL3
 */
#define CURRENTEL_EL3 (3 << 2)

/* SCTRL_EL2: System Control Register(EL2), 64-bit, Provide top level control of the
 * system, including its memory system, at EL2.
 * RES1 bits,  little-endian, caches and MMU off, no alignment checking,
 * no WXN.
 */
#define SCTLR_EL2_RESET (3 << 28 | 3 << 22 | 1 << 18 | 1 << 16 | 1 << 11 | 3 << 4)

/*
 * SPSR_EL2: Saved Program Status Register(EL2)
 *
 *
 */
#define SPSR_A    (1 << 8)    /* System Error masked */
#define SPSR_D    (1 << 9)    /* Debug masked */
#define SPSR_I    (1 << 7)    /* IRQ masked */
#define SPSR_F    (1 << 6)    /* FIQ masked */
#define SPSR_EL2H (9 << 0)    /* EL2 Handler mode */

// #define SPSR_KERNEL (SPSR_A | SPSR_D | SPSR_I | SPSR_F | SPSR_EL2H)

#define SPSR_EXEC_STATE   (1 << 4)    /* Execution state */
#define SPSR_AARCH32_MODE (0xf << 0)    /* AArch32 Mode */
#define SPSR_SUPERVISOR   (0x3 << 0)    /* Supervisor Mode */

#define SPSR_KERNEL (SPSR_A | SPSR_D | SPSR_I | SPSR_F | SPSR_EXEC_STATE | (SPSR_AARCH32_MODE & SPSR_SUPERVISOR))

#define GIC_BASE  0x1850000
#define GICD_BASE (GIC_BASE + 0x1000)
#define GICC_BASE (GIC_BASE + 0x2000)

/*
 * SCTLR_EL1 bits definitions
 */
#define SCTLR_EL1_RES1		(3 << 28 | 3 << 22 | 1 << 20 |\
				 1 << 11) /* Reserved, RES1                   */
#define SCTLR_EL1_UCI_DIS	(0 << 26) /* Cache instruction disabled       */
#define SCTLR_EL1_EE_LE		(0 << 25) /* Exception Little-endian          */
#define SCTLR_EL1_WXN_DIS	(0 << 19) /* Write permission is not XN       */
#define SCTLR_EL1_NTWE_DIS	(0 << 18) /* WFE instruction disabled         */
#define SCTLR_EL1_NTWI_DIS	(0 << 16) /* WFI instruction disabled         */
#define SCTLR_EL1_UCT_DIS	(0 << 15) /* CTR_EL0 access disabled          */
#define SCTLR_EL1_DZE_DIS	(0 << 14) /* DC ZVA instruction disabled      */
#define SCTLR_EL1_ICACHE_DIS	(0 << 12) /* Instruction cache disabled       */
#define SCTLR_EL1_UMA_DIS	(0 << 9)  /* User Mask Access disabled        */
#define SCTLR_EL1_SED_EN	(0 << 8)  /* SETEND instruction enabled       */
#define SCTLR_EL1_ITD_EN	(0 << 7)  /* IT instruction enabled           */
#define SCTLR_EL1_CP15BEN_DIS	(0 << 5)  /* CP15 barrier operation disabled  */
#define SCTLR_EL1_SA0_DIS	(0 << 4)  /* Stack Alignment EL0 disabled     */
#define SCTLR_EL1_SA_DIS	(0 << 3)  /* Stack Alignment EL1 disabled     */
#define SCTLR_EL1_DCACHE_DIS	(0 << 2)  /* Data cache disabled              */
#define SCTLR_EL1_ALIGN_DIS	(0 << 1)  /* Alignment check disabled         */
#define SCTLR_EL1_MMU_DIS	(0)       /* MMU disabled                     */

/*
 * SPSR_EL3/SPSR_EL2 bits definitions
 */
#define SPSR_EL_END_LE		(0 << 9)  /* Exception Little-endian          */
#define SPSR_EL_DEBUG_MASK	(1 << 9)  /* Debug exception masked           */
#define SPSR_EL_ASYN_MASK	(1 << 8)  /* Asynchronous data abort masked   */
#define SPSR_EL_SERR_MASK	(1 << 8)  /* System Error exception masked    */
#define SPSR_EL_IRQ_MASK	(1 << 7)  /* IRQ exception masked             */
#define SPSR_EL_FIQ_MASK	(1 << 6)  /* FIQ exception masked             */
#define SPSR_EL_T_A32		(0 << 5)  /* AArch32 instruction set A32      */
#define SPSR_EL_M_AARCH64	(0 << 4)  /* Exception taken from AArch64     */
#define SPSR_EL_M_AARCH32	(1 << 4)  /* Exception taken from AArch32     */
#define SPSR_EL_M_SVC		(0x3)     /* Exception taken from SVC mode    */
#define SPSR_EL_M_HYP		(0xa)     /* Exception taken from HYP mode    */
#define SPSR_EL_M_EL1H		(5)       /* Exception taken from EL1h mode   */
#define SPSR_EL_M_EL2H		(9)       /* Exception taken from EL2h mode   */

/*
 * CNTHCTL_EL2 bits definitions
 */
#define CNTHCTL_EL2_EL1PCEN_EN	(1 << 1)  /* Physical timer regs accessible   */
#define CNTHCTL_EL2_EL1PCTEN_EN	(1 << 0)  /* Physical counter accessible      */

/*
 * CPTR_EL2 bits definitions
 */
#define CPTR_EL2_RES1		(3 << 12 | 0x3ff)           /* Reserved, RES1 */

/*
 * CPACR_EL1 bits definitions
 */
#define CPACR_EL1_FPEN_EN	(3 << 20) /* SIMD and FP instruction enabled  */

/*
 * HCR_EL2 bits definitions
 */
#define HCR_EL2_RW_AARCH64	(1 << 31) /* EL1 is AArch64                   */
#define HCR_EL2_RW_AARCH32	(0 << 31) /* Lower levels are AArch32         */
#define HCR_EL2_HCD_DIS		(1 << 29) /* Hypervisor Call disabled         */

/*
 * Drop EL to that specified by the spsr value in register mode, at
 * the address specified in register addr.
 */
.macro drop_el mode addr
msr elr_el3, \addr
msr spsr_el3, \mode
eret		//Exception Return
.endm
