/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rtthread.h>
#include "soc.h"
#include "ax_log.h"
#include "ax_common.h"
#include "drv_pwm.h"
#include "drv_pwm_reg.h"

static uint32_t pwm_base_addr[pwm_group_max] = {
    PWM_0_BASE_ADDR,
    PWM_1_BASE_ADDR,
    PWM_2_BASE_ADDR
};

static pwm_status_t pwm_status[pwm_group_max] = {
    {.group_status = pwm_idle, .channel_status = 0x00},
    {.group_status = pwm_idle, .channel_status = 0x00},
    {.group_status = pwm_busy, .channel_status = 1 << pwm_channel_3},
};

static reg_info_t pwm_pclk_set[pwm_group_max] = {
    {1 << BIT_31, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_0, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_SET},
    {1 << BIT_1, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_SET}
};

static reg_info_t pwm_pclk_clr[pwm_group_max] = {
    {1 << BIT_31, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_CLR},
    {1 << BIT_0, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_CLR},
    {1 << BIT_1, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_CLR}
};

static reg_info_t pwm_clk_set[pwm_max] = {
    {1 << BIT_19, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_20, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_21, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_22, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_23, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_24, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_25, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_26, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_27, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_28, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_29, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET},
    {1 << BIT_30, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_SET}
};

static reg_info_t pwm_clk_clr[pwm_max] = {
    {1 << BIT_19, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_20, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_21, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_22, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_23, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_24, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_25, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_26, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_27, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_28, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_29, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR},
    {1 << BIT_30, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_1_CLR}
};

static inline void pwm_set_reg(reg_info_t reg_info)
{
    ax_writel(reg_info.reg_val, reg_info.reg_addr);
}

static int pwm_clk_init(pwm_e pwm)
{
    pwm_group_e pwm_group = pwm / pwm_channel_max;
    pwm_channel_e pwm_channel = pwm % pwm_channel_max;

    rt_spin_lock("lock");

    pwm_status_e group_status = pwm_status[pwm_group].group_status;
    uint8_t channel_status = pwm_status[pwm_group].channel_status & (1 << pwm_channel);
    if (group_status && channel_status) {
        rt_spin_unlock("lock");
        AX_LOG_ERROR("pwm[%u][%u] is already in use", pwm_group, pwm_channel);
        return -1;
    }

    if (group_status == pwm_idle) {
        /* open plck */
        pwm_set_reg(pwm_pclk_set[pwm_group]);
        /* pwm group status */
        pwm_status[pwm_group].group_status = pwm_busy;
    }

    if (channel_status == 0) {
        /* open clk */
        pwm_set_reg(pwm_clk_set[pwm]);
        /* pwm channel status */
        pwm_status[pwm_group].channel_status |= (1 << pwm_channel);
    }

    rt_spin_unlock("lock");
    return 0;
}

static void pwm_info_init(pwm_e pwm, uint32_t freq_hz, uint32_t duty)
{
    pwm_group_e pwm_group = pwm / pwm_channel_max;
    pwm_channel_e pwm_channel = pwm % pwm_channel_max;

    uint32_t cycle_cnt = PWM_CLK_SEL_FREQ_HZ / freq_hz;
    uint32_t duty_cnt = (cycle_cnt * duty) / 100;
    uint32_t low_cnt = cycle_cnt - duty_cnt;

    ax_writel(low_cnt, pwm_base_addr[pwm_group] + PWM_LOADCOUNT_OFF(pwm_channel));
    ax_writel(duty_cnt, pwm_base_addr[pwm_group] + PWM_LOADCOUNT2_OFF(pwm_channel));
    ax_writel(PWM_MODE, pwm_base_addr[pwm_group] + PWM_CONTROLREG_OFF(pwm_channel));
}

int pwm_init(pwm_e pwm, uint32_t freq_hz, uint8_t duty)
{
    if (pwm < pwm_00 || pwm >= pwm_max || pwm == pwm_20 || pwm == pwm_21
            || freq_hz >= (PWM_CLK_SEL_FREQ_HZ / 2) || duty >= 100) {
        AX_LOG_ERROR("params error, pwm[%u] freq[%u Hz] duty[%u%%]", pwm, freq_hz, duty);
        return -1;
    }

    pwm_group_e pwm_group = pwm / pwm_channel_max;
    pwm_channel_e pwm_channel = pwm % pwm_channel_max;
    if (pwm_group == 2 && pwm_channel == 3) {
        AX_LOG_ERROR("setting pwm[2][3] is forbidden");
        return -1;
    }

    int r = pwm_clk_init(pwm);
    if (r != 0) {
        AX_LOG_ERROR("init pwm[%u][%u] clk fail", pwm_group, pwm_channel);
        return r;
    }

    pwm_info_init(pwm, freq_hz, duty);

    return 0;
}

int pwm_start(pwm_e pwm)
{
    if (pwm < pwm_00 || pwm >= pwm_max || pwm == pwm_20 || pwm == pwm_21) {
        AX_LOG_ERROR("params error, pwm[%u]", pwm);
        return -1;
    }

    pwm_group_e pwm_group = pwm / pwm_channel_max;
    pwm_channel_e pwm_channel = pwm % pwm_channel_max;
    if (pwm_group == 2 && pwm_channel == 3) {
        AX_LOG_ERROR("setting pwm[2][3] is forbidden");
        return -1;
    }

    uint32_t reg_addr = pwm_base_addr[pwm_group] + PWM_CONTROLREG_OFF(pwm_channel);
    uint32_t control_reg = ax_readl(reg_addr);
    uint32_t reg_set = control_reg | PWM_EN;
    ax_writel(reg_set, reg_addr);

    return 0;
}

int pwm_stop(pwm_e pwm)
{
    if (pwm < pwm_00 || pwm >= pwm_max || pwm == pwm_20 || pwm == pwm_21) {
        AX_LOG_ERROR("params error, pwm[%u]", pwm);
        return -1;
    }

    pwm_group_e pwm_group = pwm / pwm_channel_max;
    pwm_channel_e pwm_channel = pwm % pwm_channel_max;
    if (pwm_group == 2 && pwm_channel == 3) {
        AX_LOG_ERROR("setting pwm[2][3] is forbidden");
        return -1;
    }

    uint32_t reg_addr = pwm_base_addr[pwm_group] + PWM_CONTROLREG_OFF(pwm_channel);
    uint32_t control_reg = ax_readl(reg_addr);
    uint32_t en = PWM_EN;
    uint32_t reg_set = control_reg & (~en);
    ax_writel(reg_set, reg_addr);

    return 0;
}

int pwm_deinit(pwm_e pwm)
{
    if (pwm < pwm_00 || pwm >= pwm_max || pwm == pwm_20 || pwm == pwm_21) {
        AX_LOG_ERROR("params error, pwm[%u]", pwm);
        return -1;
    }

    pwm_group_e pwm_group = pwm / pwm_channel_max;
    pwm_channel_e pwm_channel = pwm % pwm_channel_max;
    if (pwm_group == 2 && pwm_channel == 3) {
        AX_LOG_ERROR("setting pwm[2][3] is forbidden");
        return -1;
    }

    rt_spin_lock("lock");
    uint8_t chanel_status = 1 << pwm_channel;
    pwm_status[pwm_group].channel_status &= (~chanel_status);
    pwm_set_reg(pwm_clk_clr[pwm]);
    if (pwm_status[pwm_group].channel_status == 0) {
        pwm_status[pwm_group].group_status = pwm_idle;
        pwm_set_reg(pwm_pclk_clr[pwm_group]);
    }
    rt_spin_unlock("lock");

    return 0;
}
