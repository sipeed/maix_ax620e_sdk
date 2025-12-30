/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_GPIO_REG_H__
#define __DRV_GPIO_REG_H__

#include <stdint.h>

typedef enum {
    gpio_group_0 = 0,
    gpio_group_1,
    gpio_group_2,
    gpio_group_3,
    gpio_group_max
} gpio_group_e;

typedef enum {
    gpio_pin_0 = 0,
    gpio_pin_1 = 1,
    gpio_pin_2 = 2,
    gpio_pin_3 = 3,
    gpio_pin_4 = 4,
    gpio_pin_5 = 5,
    gpio_pin_6 = 6,
    gpio_pin_7 = 7,
    gpio_pin_8 = 8,
    gpio_pin_9 = 9,
    gpio_pin_10 = 10,
    gpio_pin_11 = 11,
    gpio_pin_12 = 12,
    gpio_pin_13 = 13,
    gpio_pin_14 = 14,
    gpio_pin_15 = 15,
    gpio_pin_16 = 16,
    gpio_pin_17 = 17,
    gpio_pin_18 = 18,
    gpio_pin_19 = 19,
    gpio_pin_20 = 20,
    gpio_pin_21 = 21,
    gpio_pin_22 = 22,
    gpio_pin_23 = 23,
    gpio_pin_24 = 24,
    gpio_pin_25 = 25,
    gpio_pin_26 = 26,
    gpio_pin_27 = 27,
    gpio_pin_28 = 28,
    gpio_pin_29 = 29,
    gpio_pin_30 = 30,
    gpio_pin_31 = 31,
    gpio_pin_max
} gpio_pin_e;

typedef struct {
    uint32_t dr : 1;
    uint32_t ddr : 1;
    uint32_t reserved0 : 1;
    uint32_t inten : 1;
    uint32_t intmask: 1;
    uint32_t inttype_level : 1;
    uint32_t int_polarity : 1;
    uint32_t debounce : 1;
    uint32_t eoi : 1;
    uint32_t int_bothedge : 1;
    uint32_t reserved1 : 22;
} porta_func_t;

typedef struct {
    volatile uint32_t protect_access;
    volatile porta_func_t portax_func[32];
    volatile uint32_t intstatus_s;
    volatile uint32_t raw_intstatus_s;
    volatile uint32_t ext_porta;
    volatile uint32_t id_code;
    volatile uint32_t ls_sync;
    volatile uint32_t ver_id_code;
    volatile uint32_t config_reg2;
    volatile uint32_t config_reg1;
    volatile uint32_t instatus_ns;
    volatile uint32_t raw_instatus_ns;
} gpio_regs_t;

#endif //__DRV_GPIO_REG_H__
