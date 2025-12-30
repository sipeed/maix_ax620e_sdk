/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <common.h>
#include <cpu_func.h>
#include "rtthread.h"

#define DDR_RAM_ADDR_START 	0x46000000
#define DDR_RAM_SIZE        0x300000
#define RISCV_PC_ADDR_START	(0x46000000)

#define COMM_SYS_GLB                     0x02340000
#define COMM_SYS_GLB_SW_RST_0_SET        (COMM_SYS_GLB + 0x58)
#define COMM_SYS_GLB_RISCV_BASE_ADDR     (COMM_SYS_GLB + 0x24C)
#define COMM_SYS_GLB_CLK_EB_1_SET        (COMM_SYS_GLB + 0x34)
#define COMM_SYS_GLB_SW_RST_0_CLR        (COMM_SYS_GLB + 0x5C)
#define COMM_SYS_GLB_CLK_MUX1_SET        (COMM_SYS_GLB + 0x10)
#define COMM_SYS_GLB_SW_RST_0_CLR        (COMM_SYS_GLB + 0x5C)

static void RegWrite(uint64_t addr, uint32_t val)
{
    *(volatile uint32_t*)addr = val;
}

static inline void ax_writel(u32 value, u64 addr)
{
    *(volatile u32 *)addr = value;
}

void riscv_boot_up(void)
{
    /* reset riscv */
    ax_writel(BIT(16), COMM_SYS_GLB_SW_RST_0_SET);
    /* riscv_base_addr */
    ax_writel(RISCV_PC_ADDR_START, COMM_SYS_GLB_RISCV_BASE_ADDR);
    /* clk_riscv_eb set */
    ax_writel(BIT(8), COMM_SYS_GLB_CLK_EB_1_SET);
    /* riscv_sw_rst clear field */
    ax_writel(BIT(17), COMM_SYS_GLB_SW_RST_0_CLR);
    /* clk_riscv_bus_sel set cpll_416m */
    ax_writel(BIT(25), COMM_SYS_GLB_CLK_MUX1_SET);
    /* riscv_hold_sw_rst clear field */
    ax_writel(BIT(16), COMM_SYS_GLB_SW_RST_0_CLR);
}

static int _do_riscv_boot(uint64_t ADDR, phys_size_t LEN, phys_addr_t FW_ADDR, phys_size_t FW_LEN)
{
    uint64_t ramBase = ADDR;
    for (int i = 0; i < (FW_LEN / 4); i++){
        RegWrite(ramBase + i * 4, *((u32*)FW_ADDR + i));
    }
    flush_dcache_all();

    riscv_boot_up();

    return 0;
}

int do_riscv_boot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    int ret = 0;
    printf("start risc-v boot\n");
    ret = _do_riscv_boot(DDR_RAM_ADDR_START, DDR_RAM_SIZE, (phys_addr_t)rtthread_bin, rtthread_bin_len);
    if (ret != 0) {
        printf("riscv test boot up fail");
        return -1;
    }
    printf("start risc-v success\n");
    return 0;
}

int boot_riscv(void)
{
    printf("to boot risc-v\n");
    return _do_riscv_boot(DDR_RAM_ADDR_START, DDR_RAM_SIZE, (phys_addr_t)rtthread_bin, rtthread_bin_len);
}

U_BOOT_CMD(riscv_boot, 1, 0, do_riscv_boot,
		"risc-v boot", NULL);
