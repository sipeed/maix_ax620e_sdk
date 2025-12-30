/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <finsh.h>

#include "ax_log.h"
#include "drv_mailbox.h"
#include "board.h"
#include "mc20e_e907_interrupt.h"
#include "ax_common.h"
#include "drv_mailbox_reg.h"

static rt_list_t callback_list_head;

static void mailbox_clk_enable(void)
{
    ax_writel(1UL << PCLK_MAILBOX_EB_SHIFT, FLASH_SYS_GLB_BASE + FLASH_SYS_GLB_CLK_EB_1_SET_OFFSET);
}

static void mailbox_clk_disable(void)
{
    ax_writel(1UL << PCLK_MAILBOX_EB_SHIFT, FLASH_SYS_GLB_BASE + FLASH_SYS_GLB_CLK_EB_1_CLR_OFFSET);
}

static void mailbox_reset(void)
{
    ax_writel(1UL << MAILBOX_SW_PRST_SHIFT, FLASH_SYS_GLB_BASE + FLASH_SYS_GLB_SW_RST_0_SET_OFFSET);
    ax_udelay(200);
    ax_writel(1UL << MAILBOX_SW_PRST_SHIFT, FLASH_SYS_GLB_BASE + FLASH_SYS_GLB_SW_RST_0_CLR_OFFSET);
}

static int mbox_handle_msg(mbox_msg_t *msg)
{
    mbox_callback_info_t *info = NULL;
    mbox_callback_info_t *pos;
    rt_list_for_each_entry(pos, &callback_list_head, list) {
        if (pos->id == msg->id) {
            info = pos;
            info->callback(msg, info->data);
            break;
        }
    }

    if (info == NULL) {
        AX_LOG_ERROR("callback not found");
        return -1;
    }

    return 0;
}

static void mailbox_irq_handler(int irqno, void *param)
{
    uint32_t status;

    status = ax_readl(MAILBOX_BASE + INT_STATUS_REG_OFFSET);
    ax_writel(status & 0xFUL, MAILBOX_BASE + INT_CLEAR_REG_OFFSET);

    uint32_t channel = (status >> 16) & 0x3FUL;
    uint32_t info = ax_readl(MAILBOX_BASE + INFO_REG_X(channel));

    uint32_t send_id = (info >> 24) & 0xFUL;
    uint32_t size = info & 0xFF;

    mbox_msg_t msg;
    uint32_t *ptr = (uint32_t *)&msg;
    switch (send_id) {
    case AX_MAILBOX_MASTER_ID_ARM0:
        if (size < 4) {
            msg.id = 0xff;
        }
        for(int i = 0; i < (size / 4); i++) {
            ptr[i] = ax_readl(MAILBOX_BASE + DATA_REG_X(channel));
        }
        mbox_handle_msg(&msg);
        break;
    default:
        AX_LOG_ERROR("undefined master id %u", send_id);
    }
}

static int mbox_request_channel(void)
{
    uint32_t channel = ax_readl(MAILBOX_BASE + REG_QUERY_REG_OFFSET);

    if (channel == 0xFFFFFFFFUL) {
        return -1;
    } else {
        return channel;
    }
}

static int mbox_send_message(int channel, void *data, rt_size_t size)
{
    if (size > 32)
        size = 32;
    for (int i = 0; i < (size / 4); i++) {
        ax_writel(((uint32_t*)data)[i], MAILBOX_BASE + DATA_REG_X(channel));
    }

    uint32_t info = (AX_MAILBOX_MASTER_ID_ARM0 << 28) | (AX_MAILBOX_MASTER_ID_RISCV << 24) | size;
    ax_writel(info, MAILBOX_BASE + INFO_REG_X(channel));

    return size;
}

int mbox_auto_send_message(mbox_msg_t *msg)
{
    if (msg == NULL) {
        AX_LOG_ERROR("params error");
        return -1;
    }

    int channel = mbox_request_channel();
    if (channel == -1) {
        AX_LOG_ERROR("request mailbox channel fail");
        return -1;
    }
    mbox_send_message(channel, msg, 32);

    return 0;
}

int mbox_register_callback(mbox_callback_t callback, void *data, uint8_t id)
{
    rt_base_t level = rt_spin_lock_irqsave("lock");
    mbox_callback_info_t *pos;
    rt_list_for_each_entry(pos, &callback_list_head, list) {
        if (pos->id == id) {
            rt_spin_unlock_irqrestore("lock", level);
            AX_LOG_ERROR("callback id %u has aready exist", id);
            return -1;
        }
    }

    mbox_callback_info_t *info = rt_malloc(sizeof(mbox_callback_info_t));
    info->callback = callback;
    info->data = data;
    info->id = id;
    rt_list_insert_before(&callback_list_head, &info->list);

    rt_spin_unlock_irqrestore("lock", level);
    return 0;
}

int mbox_unregister_callback(uint8_t id)
{
    mbox_callback_info_t *info = RT_NULL;
    mbox_callback_info_t *pos, *tmp;

    rt_base_t level = rt_spin_lock_irqsave("lock");
    rt_list_for_each_entry_safe(pos, tmp, &callback_list_head, list) {
        if (pos->id == id) {
            info = pos;
            rt_list_remove(&info->list);
            break;
        }
    }

    if (info != NULL) {
        rt_free(info);
    } else {
        rt_spin_unlock_irqrestore("lock", level);
        AX_LOG_ERROR("callback id %u is not exist", id);
        return -1;
    }

    rt_spin_unlock_irqrestore("lock", level);
    return 0;
}

int mc20e_mailbox_init(void)
{
    rt_list_init(&callback_list_head);

    mailbox_clk_enable();
    mailbox_reset();

    mc20e_e907_interrupt_install(INT_MAILBOX_INT2, mailbox_irq_handler, NULL, "mailbox");
    mc20e_e907_interrupt_umask(INT_MAILBOX_INT2);
    return 0;
}

int mc20e_mailbox_deinit(void)
{
    mailbox_clk_disable();
    return 0;
}

INIT_DEVICE_EXPORT(mc20e_mailbox_init);
