/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-20     zx.chen      CSI Core Peripheral Access Layer Header File for
 *                             CSKYSOC Device Series
 */

#ifndef __MC20E_E907_SOC_H__
#define __MC20E_E907_SOC_H__

#include <stdint.h>
#include <csi_core.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AX_RISCV_LOAD_ROOTFS_SUPPORT
#define RAMDISK_FLASH_BASE 0x4e000000
#define RAMDISK_DDR_BASE   0x45dffc00
#endif

#ifndef AX_RISCV_LOAD_MODEL_SUPPORT
#define MODEL_FLASH_BASE   0xBF0000
#define MODEL_DDR_BASE     0x46AFDC00
#endif

#ifndef RISCV_DDR_BASE
#define RISCV_DDR_BASE  0x47c00000
#endif

#ifndef RISCV_MEM_LEN_MB
#define RISCV_MEM_LEN_MB   2
#endif

#ifndef ISP_RESERVED_DDR_BASE
#define ISP_RESERVED_DDR_BASE  0x4e000000
#endif

#ifndef ISP_RESERVED_DDR_END
#define ISP_RESERVED_DDR_END  0x50000000
#endif

#ifdef HAPS
#define IHS_VALUE    (10000000)
#else
#define IHS_VALUE    (24000000)
#endif

#ifndef EHS_VALUE
#define EHS_VALUE    (20000000)
#endif

/* -------------------------  Interrupt Number Definition  ------------------------ */
typedef enum IRQn {
    NMI_EXPn                        =   -2,      /* NMI Exception */
    /* ----------------------  SmartL Specific Interrupt Numbers  --------------------- */
    Machine_Software_IRQn           =   3,      /* Machine software interrupt */
    User_Timer_IRQn                 =   4,      /* User timer interrupt */
    Supervisor_Timer_IRQn           =   5,      /* Supervisor timer interrupt */
    CORET_IRQn                      =   7,      /* core Timer Interrupt */
    Machine_External_IRQn           =   11,     /* Machine external interrupt */
    ALL_PERIPH_IRQn                 =   16,     /* uart Interrupt */
} IRQn_Type;

/* ================================================================================ */
/* ================       Device Specific Peripheral Section       ================ */
/* ================================================================================ */

/* flash_sys_glb */
#define FLASH_SYS_GLB_BASE                   0x10030000
#define FLASH_SYS_GLB_CLK_EB_1_OFFSET        0x08
#define FLASH_SYS_GLB_CLK_EB_1_SET_OFFSET    0x4008
#define FLASH_SYS_GLB_CLK_EB_1_CLR_OFFSET    0x8008
#define PCLK_MAILBOX_EB_SHIFT                15
#define FLASH_SYS_GLB_SW_RST_0_OFFSET        0x14
#define FLASH_SYS_GLB_SW_RST_0_SET_OFFSET    0x4014
#define FLASH_SYS_GLB_SW_RST_0_CLR_OFFSET    0x8014
#define MAILBOX_SW_PRST_SHIFT                13

/* periph_sys_glb */
#define PERIPH_SYS_GLB_ADDR    0x4870000
#define PERIPH_CLK_EB_0        0x04
#define PERIPH_CLK_EB_0_SET    0xb0
#define PERIPH_CLK_EB_0_CLR    0xb4
#define PERIPH_CLK_EB_1        0x08
#define PERIPH_CLK_EB_1_SET    0xb8
#define PERIPH_CLK_EB_1_CLR    0xbc
#define PERIPH_CLK_EB_2        0x0c
#define PERIPH_CLK_EB_2_SET    0xc0
#define PERIPH_CLK_EB_2_CLR    0xc4
#define PERIPH_CLK_EB_3        0x10
#define PERIPH_CLK_EB_3_SET    0xc8
#define PERIPH_CLK_EB_3_CLR    0xcc
#define PERIPH_SW_RST_0        0x18
#define PERIPH_SW_RST_0_SET    0xd8
#define PERIPH_SW_RST_0_CLR    0xdc
#define PERIPH_SW_RST_1        0x1c
#define PERIPH_SW_RST_1_SET    0xe0
#define PERIPH_SW_RST_1_CLR    0xe4
#define PERIPH_CLK_MUX_0       0x00
#define PERIPH_CLK_MUX_0_SET   0xa8
#define PERIPH_CLK_MUX_0_CLR   0xac


/* ================================================================================ */
/* ================              Peripheral memory map             ================ */
/* ================================================================================ */

/* interrupt  */
#define RISCV_INT_REG_BASE    0x234027c

/* UART 0/1/2/3/4/5 */
#define UART0_BASE_ADDR       0x4880000
#define UART1_BASE_ADDR       0x4881000
#define UART2_BASE_ADDR       0x4882000
#define UART3_BASE_ADDR       0x6080000
#define UART4_BASE_ADDR       0x6081000
#define UART5_BASE_ADDR       0x6082000

/* mailbox */
#define MAILBOX_BASE    0x10430000

/* sw int */
#define AXERA_SW_INT0_BASE    0x2310000
#define AXERA_SW_INT1_BASE    0x2310040
#define AXERA_SW_INT2_BASE    0x2310080
#define AXERA_SW_INT3_BASE    0x23100c0

/* GPIO group 0/1/2/3 */
#define GPIO_GP0_BASE_ADDR    0x04800000
#define GPIO_GP1_BASE_ADDR    0x04801000
#define GPIO_GP2_BASE_ADDR    0x06000000
#define GPIO_GP3_BASE_ADDR    0x06001000

/* I2C 0/1/2/3/4/5/6/7 */
#define I2C0_BASE_ADDR    0x04850000
#define I2C1_BASE_ADDR    0x04851000
#define I2C2_BASE_ADDR    0x04852000
#define I2C3_BASE_ADDR    0x04853000
#define I2C4_BASE_ADDR    0x04854000
#define I2C5_BASE_ADDR    0x04855000
#define I2C6_BASE_ADDR    0x04856000
#define I2C7_BASE_ADDR    0x04857000

/* timer64 0/1 */
#define TMR64_0_BASE_ADDR    0x04820000
#define TMR64_0_FREQ_M       24
#define TMR64_1_BASE_ADDR    0x06020000

/* DW_apb_timer and PWM */
#define APB_TMR32_0_BASE_ADDR     0x4830000
#define APB_TMR32_1_BASE_ADDR     0x6030000
#define PWM_0_BASE_ADDR           0x6060000
#define PWM_1_BASE_ADDR           0x6061000
#define PWM_2_BASE_ADDR           0x6062000

/* dummy register for verify or other use */
#define AX_DUMMY_SW4_ADDR           0x2340200
#define AX_VERIFY_MAGIC_AA          0xaa000000
#define AX_VERIFY_MAGIC_55          0x55000000
#define SW4_ENTRY_LOWPOWER_MAGIC    0x6c706d64

/* ive */
#define AX_IVE_BASE     0x4406000

/* mm glb */
#define AX_MM_GLB_BASE  0x4430000

/* ================================================================================ */
/* ================             Peripheral declaration             ================ */
/* ================================================================================ */

#ifdef __cplusplus
}
#endif

#endif  // __MC20E_E907_SOC_H__
