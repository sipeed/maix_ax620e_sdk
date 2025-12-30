/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _DRV_I2C_H_
#define _DRV_I2C_H_

#include <drivers/i2c.h>
#include <stdint.h>

typedef enum {
    i2c_channel_0 = 0,
    i2c_channel_1,
    i2c_channel_2,
    i2c_channel_3,
    i2c_channel_4,
    i2c_channel_5,
    i2c_channel_6,
    i2c_channel_7,
    i2c_channel_max
} i2c_channel_e;

typedef enum {
    i2c_freq_100k = 100000,
    i2c_freq_400k = 400000,
    i2c_freq_800k = 800000,
    i2c_freq_1M = 1000000
} i2c_freq_e;

int i2c_init(i2c_channel_e channel, const char *name, i2c_freq_e freq);
int32_t i2c_wrtie_reg(struct rt_i2c_bus_device *i2c_dev,
                      rt_uint8_t addr,
                      rt_uint32_t reg,
                      rt_uint8_t* src,
                      rt_uint32_t len,
                      rt_uint32_t is_reg_addr_16bits);
int32_t i2c_read_reg(struct rt_i2c_bus_device *i2c_dev,
                     rt_uint8_t addr,
                     rt_uint32_t reg,
                     rt_uint8_t* dst,
                     rt_uint32_t len,
                     rt_uint32_t is_reg_addr_16bits);
#endif
