/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

#include <stdint.h>

typedef enum {
    gpio_mode_output = 0,
    gpio_mode_input,
    gpio_mode_max
} gpio_mode_e;

typedef enum {
    gpio_low = 0,
    gpio_high
} gpio_value_e;

typedef enum {
    gpio_int_mode_rising = 0,
    gpio_int_mode_falling,
    gpio_int_mode_rising_falling,
    gpio_int_mode_high_level,
    gpio_int_mode_low_level,
    gpio_int_mode_max
} gpio_int_mode_e;

int gpio_set_mode(uint32_t gpio_num, gpio_mode_e gpio_mode);
int gpio_set_value(uint32_t gpio_num, gpio_value_e value);
gpio_value_e gpio_get_value(uint32_t gpio_num);
int gpio_set_raw_int_mode(uint32_t gpio_num, gpio_int_mode_e int_mode);
int gpio_get_raw_int_status(uint32_t gpio_num);
int gpio_clear_raw_int_status(uint32_t gpio_num);

#endif //__DRV_GPIO_H__
