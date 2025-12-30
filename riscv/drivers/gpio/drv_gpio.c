/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rtdevice.h>
#include "soc.h"
#include "drv_gpio.h"
#include "drv_gpio_reg.h"
#include "ax_log.h"

#define GPIO_NUM_MAX  (gpio_group_max * gpio_pin_max)

static gpio_regs_t *gpio_base[gpio_group_max] = {
    (gpio_regs_t*)GPIO_GP0_BASE_ADDR,
    (gpio_regs_t*)GPIO_GP1_BASE_ADDR,
    (gpio_regs_t*)GPIO_GP2_BASE_ADDR,
    (gpio_regs_t*)GPIO_GP3_BASE_ADDR
};

int gpio_set_mode(uint32_t gpio_num, gpio_mode_e mode)
{
    if (gpio_num >= GPIO_NUM_MAX || mode < 0 || mode >= gpio_mode_max) {
        AX_LOG_ERROR("params error");
        return RT_EINVAL;
    }

    gpio_group_e group = gpio_num / gpio_pin_max;
    gpio_pin_e pin = gpio_num % gpio_pin_max;

    gpio_regs_t *gpio_regs = gpio_base[group];
    volatile porta_func_t *portax_func = &gpio_regs->portax_func[pin];

    switch (mode) {
    case gpio_mode_output:
        portax_func->ddr = 1;
        break;

    case gpio_mode_input:
        portax_func->ddr = 0;
        break;
    default:
        AX_LOG_ERROR("pin mode %u error", mode);
        return RT_EINVAL;
    }

    return 0;
}

int gpio_set_value(uint32_t gpio_num, gpio_value_e value)
{
    if (gpio_num >= GPIO_NUM_MAX) {
        AX_LOG_ERROR("params error");
        return RT_EINVAL;
    }

    gpio_group_e group = gpio_num / gpio_pin_max;
    gpio_pin_e pin = gpio_num % gpio_pin_max;

    gpio_regs_t *gpio_regs = gpio_base[group];
    volatile porta_func_t *portax_func = &gpio_regs->portax_func[pin];
    portax_func->dr = value;

    return 0;
}

gpio_value_e gpio_get_value(uint32_t gpio_num)
{
    if (gpio_num >= GPIO_NUM_MAX) {
        AX_LOG_ERROR("params error");
        return RT_EINVAL;
    }

    gpio_group_e group = gpio_num / gpio_pin_max;
    gpio_pin_e pin = gpio_num % gpio_pin_max;
    gpio_regs_t *gpio_regs = gpio_base[group];

    return ((gpio_regs->ext_porta >> pin) & 0x01);
}

int gpio_set_raw_int_mode(uint32_t gpio_num, gpio_int_mode_e int_mode)
{
    if (gpio_num >= GPIO_NUM_MAX || int_mode < 0 || int_mode >= gpio_int_mode_max) {
        AX_LOG_ERROR("params error");
        return RT_EINVAL;
    }

    gpio_group_e group = gpio_num / gpio_pin_max;
    gpio_pin_e pin = gpio_num % gpio_pin_max;

    gpio_regs_t *gpio_regs = gpio_base[group];
    volatile porta_func_t *portax_func = &gpio_regs->portax_func[pin];

    switch (int_mode) {
    case gpio_int_mode_rising:
        portax_func->inttype_level = 1;
        portax_func->int_polarity = 1;
        portax_func->int_bothedge = 0;
        break;
    case gpio_int_mode_falling:
        portax_func->inttype_level = 1;
        portax_func->int_polarity = 0;
        portax_func->int_bothedge = 0;
        break;
    case gpio_int_mode_rising_falling:
        portax_func->inttype_level = 0;
        portax_func->int_polarity = 0;
        portax_func->int_bothedge = 1;
        break;
    case gpio_int_mode_high_level:
        portax_func->inttype_level = 0;
        portax_func->int_polarity = 1;
        portax_func->int_bothedge = 0;
        break;
    case gpio_int_mode_low_level:
        portax_func->inttype_level = 0;
        portax_func->int_polarity = 0;
        portax_func->int_bothedge = 0;
        break;
    default:
        AX_LOG_ERROR("int mode %u error", int_mode);
        return RT_EINVAL;
    }

    portax_func->debounce = 1;
    portax_func->intmask = 1;
    portax_func->inten = 1;

    return 0;
}

int gpio_get_raw_int_status(uint32_t gpio_num)
{
    int r;

    if (gpio_num >= GPIO_NUM_MAX) {
        AX_LOG_ERROR("params error");
        return RT_EINVAL;
    }

    gpio_group_e group = gpio_num / gpio_pin_max;
    gpio_pin_e pin = gpio_num % gpio_pin_max;

    gpio_regs_t *gpio_regs = gpio_base[group];
    uint32_t protect_access = gpio_regs->protect_access;
    if (protect_access & (1 << pin)) {
        r = (gpio_regs->raw_intstatus_s >> pin) & 1;
    } else {
        r= (gpio_regs->raw_instatus_ns >> pin) & 1;
    }

    return r;
}

int gpio_clear_raw_int_status(uint32_t gpio_num)
{
    if (gpio_num >= GPIO_NUM_MAX) {
        AX_LOG_ERROR("params error");
        return RT_EINVAL;
    }

    gpio_group_e group = gpio_num / gpio_pin_max;
    gpio_pin_e pin = gpio_num % gpio_pin_max;

    gpio_regs_t *gpio_regs = gpio_base[group];
    volatile porta_func_t *portax_func = &gpio_regs->portax_func[pin];

    portax_func->eoi = 1;
    portax_func->eoi = 0;

    return 0;
}
