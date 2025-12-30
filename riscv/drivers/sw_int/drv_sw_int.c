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
#include "mc20e_e907_interrupt.h"
#include "ax_log.h"
#include "drv_sw_int.h"
#include "drv_sw_int_reg.h"
#include "soc.h"
#include "ax_common.h"

static sw_int_regs_t *sw_int_base[sw_int_group_max] = {
    (sw_int_regs_t*)AXERA_SW_INT0_BASE,
    (sw_int_regs_t*)AXERA_SW_INT1_BASE,
    (sw_int_regs_t*)AXERA_SW_INT2_BASE,
    (sw_int_regs_t*)AXERA_SW_INT3_BASE
};
static rt_list_t sw_int_cb_list_head[sw_int_group_max];

static void sw_int_verify_riscv_AA(void *param)
{
    ax_writel(AX_VERIFY_MAGIC_AA, AX_DUMMY_SW4_ADDR);
}

static void sw_int_verify_riscv_55(void *param)
{
    ax_writel(AX_VERIFY_MAGIC_55, AX_DUMMY_SW4_ADDR);
}

static int sw_int_config(sw_int_group_e group, sw_int_channel_e channel, bool enable)
{
    if (group < 0 || group >= sw_int_group_max || channel < 0 || channel >= sw_int_channel_max) {
        AX_LOG_ERROR("params error, group[%d] channel[%d]", group, channel);
        return -1;
    }

    uint32_t channel_val = 1 << channel;
    sw_int_regs_t *sw_int_regs = sw_int_base[group];
    sw_int_regs->int_raw_clr = channel_val;
    if (enable == true) {
        sw_int_regs->int_en_set = channel_val;
    } else {
        sw_int_regs->int_en_clr = channel_val;
    }

    return 0;
}

static int sw_int_handle(sw_int_group_e group, uint32_t channel_val)
{
    sw_int_cb_info_t *info = NULL;
    sw_int_cb_info_t *pos;
    rt_list_for_each_entry(pos, &sw_int_cb_list_head[group], list) {
        if (pos->group == group && (1 << pos->channel) == channel_val) {
            info = pos;
            info->cb(info->param);
            break;
        }
    }

    if (info == NULL) {
        AX_LOG_ERROR("callback not found, group %u channel_val 0x%x", group, channel_val);
        return -1;
    }

    return 0;
}

static void sw_int_0_irq_handler(int irqno, void *param)
{
    sw_int_regs_t *sw_int_regs = sw_int_base[sw_int_group_0];

    uint32_t channel_val = sw_int_regs->int_sta;
    sw_int_regs->int_raw_clr = channel_val;
    AX_LOG_INFO("irqno = %d, channel_val = 0x%x", irqno, channel_val);
    sw_int_handle(sw_int_group_0, channel_val);
}

static void sw_int_1_irq_handler(int irqno, void *param)
{
    sw_int_regs_t *sw_int_regs = sw_int_base[sw_int_group_1];

    uint32_t channel_val = sw_int_regs->int_sta;
    sw_int_regs->int_raw_clr = channel_val;
    AX_LOG_INFO("irqno = %d, channel_val = 0x%x", irqno, channel_val);
    sw_int_handle(sw_int_group_1, channel_val);
}

static void sw_int_2_irq_handler(int irqno, void *param)
{
    sw_int_regs_t *sw_int_regs = sw_int_base[sw_int_group_2];

    uint32_t channel_val = sw_int_regs->int_sta;
    sw_int_regs->int_raw_clr = channel_val;
    AX_LOG_INFO("irqno = %d, channel_val = 0x%x", irqno, channel_val);
    sw_int_handle(sw_int_group_2, channel_val);
}

static void sw_int_3_irq_handler(int irqno, void *param)
{
    sw_int_regs_t *sw_int_regs = sw_int_base[sw_int_group_3];

    uint32_t channel_val = sw_int_regs->int_sta;
    sw_int_regs->int_raw_clr = channel_val;
    AX_LOG_INFO("irqno = %d, channel_val = 0x%x", irqno, channel_val);
    sw_int_handle(sw_int_group_3, channel_val);
}

void riscv_sw_int_trigger(sw_int_group_e group, sw_int_channel_e channel)
{
	uint32_t trigger_reg_addr = (uint32_t)sw_int_base[group] + SW_INT_TRIGGER_OFFSET;
	uint32_t trigger_value = 1 << channel;
	ax_writel(trigger_value, trigger_reg_addr);
}

int sw_int_cb_register(sw_int_group_e group, sw_int_channel_e channel, sw_int_cb_t cb, void *param)
{
    if (group < 0 || group >= sw_int_group_max ||
        channel < 0 || channel >= sw_int_channel_max || cb == RT_NULL) {
        AX_LOG_ERROR("params error, group[%d] channel[%d] cb[%p]", group, channel, cb);
        return -1;
    }

    rt_base_t level = rt_spin_lock_irqsave("lock");
    sw_int_cb_info_t *info;
    rt_list_for_each_entry(info, &sw_int_cb_list_head[group], list) {
        if (info->channel == channel) {
            rt_spin_unlock_irqrestore("lock", level);
            AX_LOG_ERROR("group[%d] channel[%d] is busy", group, channel);
            return -1;
        }
    }

    info = rt_malloc(sizeof(sw_int_cb_info_t));
    info->group = group;
    info->channel = channel;
    info->cb = cb;
    info->param = param;
    rt_list_insert_before(&sw_int_cb_list_head[group], &info->list);
    sw_int_config(group, channel, true);
    rt_spin_unlock_irqrestore("lock", level);

    return 0;
}

int sw_int_cb_unregister(sw_int_group_e group, sw_int_channel_e channel)
{
    if (group < 0 || group >= sw_int_group_max || channel < 0 || channel >= sw_int_channel_max) {
        AX_LOG_ERROR("params error, group[%d] channel[%d]", group, channel);
        return -1;
    }

    rt_base_t level = rt_spin_lock_irqsave("lock");
    sw_int_cb_info_t *info = RT_NULL;
    sw_int_cb_info_t *pos, *tmp;
    rt_list_for_each_entry_safe(pos, tmp, &sw_int_cb_list_head[group], list) {
        if (pos->group == group && pos->channel == channel) {
            info = pos;
            rt_list_remove(&info->list);
            break;
        }
    }
    if (info != RT_NULL) {
        sw_int_config(group, channel, false);
        rt_free(info);
    } else {
        AX_LOG_ERROR("grourp[%u] channel[%u] is idle, no need to unregister", group, channel);
        rt_spin_unlock_irqrestore("lock", level);
        return -1;
    }
    rt_spin_unlock_irqrestore("lock", level);

    return 0;
}

static int sw_int_init(void)
{
    for (int i = 0; i < sw_int_group_max; i++) {
        rt_list_init(&sw_int_cb_list_head[i]);
    }

    mc20e_e907_interrupt_install(INT_SW_INT_O_0, sw_int_0_irq_handler, NULL, "sw_int_0");
    mc20e_e907_interrupt_install(INT_SW_INT_O_1, sw_int_1_irq_handler, NULL, "sw_int_1");
    mc20e_e907_interrupt_install(INT_SW_INT_O_2, sw_int_2_irq_handler, NULL, "sw_int_2");
    mc20e_e907_interrupt_install(INT_SW_INT_O_3, sw_int_3_irq_handler, NULL, "sw_int_3");

    mc20e_e907_interrupt_umask(INT_SW_INT_O_0);
    mc20e_e907_interrupt_umask(INT_SW_INT_O_1);
    mc20e_e907_interrupt_umask(INT_SW_INT_O_2);
    mc20e_e907_interrupt_umask(INT_SW_INT_O_3);

    sw_int_cb_register(sw_int_group_3, sw_int_channel_30, sw_int_verify_riscv_AA, RT_NULL);
    sw_int_cb_register(sw_int_group_3, sw_int_channel_31, sw_int_verify_riscv_55, RT_NULL);

    return 0;
}

INIT_DEVICE_EXPORT(sw_int_init);
