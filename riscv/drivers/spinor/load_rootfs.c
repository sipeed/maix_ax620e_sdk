/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#define AX_LOG_TAG

#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include "ax_common.h"
#include "drv_sw_int.h"
#include "boot.h"
#include "dma.h"
#include "drv_timer64.h"
#include "ax_timestamp.h"
#include "load_rootfs.h"

#define LOAD_ROOTFS_READY   BIT(5)
#define LOAD_ROOTFS_DONE    BIT(0)
#define LOAD_MODEL_DONE     BIT(1)

static rt_sem_t load_sem;
static int32_t debug_base;

void debug_trace(int flag)
{
    int val;
    val = ax_readl(debug_base);
    val |= flag;
    ax_writel(val, debug_base);
}

static void wakeup_load_rootfs(void *param)
{
    rt_sem_release(load_sem);
}

static void load_rootfs_thread(void* parameter)
{
    load_sem = rt_sem_create("load_sem", 0, RT_IPC_FLAG_PRIO);
    sw_int_cb_register(sw_int_group_3, sw_int_channel_29, wakeup_load_rootfs, RT_NULL);
    ax_writel(LOAD_ROOTFS_READY, AX_DUMMY_SW4_ADDR);

    rt_sem_take(load_sem, RT_WAITING_FOREVER);
    AX_LOG_INFO("loadrootfs start");
    ax_timestamp(STAMP_RISCV_START_LOAD_ROOTFS);
    axi_dma_per_int_init();
    debug_base =  DEBUG_ROOTFS_TRACE_BASE;
    debug_trace(ROOTFS_START);
#ifdef SUPPORT_RECOVERY
    int ret = flash_boot(RAMDISK_FLASH_BASE, 0, RAMDISK_DDR_BASE);
    if (ret) {
        AX_LOG_INFO("loadrootfs failed restart into recovery mode.");
        ax_writel(0x800, TOP_CHIPMODE_GLB_BACKUP0_SET);
        ax_writel(BIT(0), COMM_ABORT_CFG);
    }
#else
    flash_boot(RAMDISK_FLASH_BASE, 0, RAMDISK_DDR_BASE);
#endif
#ifdef AX_RISCV_LOAD_MODEL_SUPPORT
    debug_base =  DEBUG_MODEL_TRACE_BASE;
    debug_trace(MODEL_START);
    flash_boot(MODEL_FLASH_BASE, 0, MODEL_DDR_BASE);
#endif
    axi_dma_per_int_deinit();
    ax_timestamp(STAMP_RISCV_END_LOAD_ROOTFS);
    ax_writel(LOAD_ROOTFS_DONE, AX_DUMMY_SW4_ADDR);
    AX_LOG_INFO("loadrootfs finished");

    sw_int_cb_unregister(sw_int_group_3, sw_int_channel_29);
    rt_sem_delete(load_sem);
}

int read_rootfs(void)
{
    rt_memset((int *)DEBUG_ROOTFS_TRACE_BASE, 0, 0x20);
    rt_thread_t tid = rt_thread_create("load_rootfs", load_rootfs_thread, RT_NULL, 2048, 5, 5);
    if(tid == RT_NULL) {
        AX_LOG_ERROR("create load_rootfs_thread failed!\n");
        return -1;
    }
    rt_thread_startup(tid);

    return 0;
}
