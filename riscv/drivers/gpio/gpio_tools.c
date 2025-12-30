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
#include <rthw.h>
#include <rtdevice.h>
#include <stdlib.h>
#include <string.h>
#include "drv_gpio.h"

#define GPIO_TOOL_SET_INT   0
#define GPIO_TOOL_GET_INT   1

typedef enum {
    gpio_type_in = 0,
    gpio_type_out,
    gpio_type_int
} gpio_type_e;

typedef struct {
    gpio_type_e type;
    rt_uint32_t num;
    union {
        rt_uint32_t out_val;
        rt_uint32_t int_ops;
    };
} gpio_test_info_t;

static void print_usage()
{
    rt_kprintf("Usage: gpio_tools in/out/int  gpio_num value\n"
               "    in/out/int:      input/output/interrupt mode\n"
               "    gpio_num:   gpio number\n"
               "    value:      value for output, operation for interrupt, invalid for input\n");
}

static int gpio_parse_args(rt_int32_t argc, char* argv[], gpio_test_info_t *info)
{
    if (info == NULL) {
        return -1;
    }

    if(argc != 4) {
        return -1;
    }

    if (strcmp(argv[1], "in") == 0) {
        info->type = gpio_type_in;
    } else if (strcmp(argv[1],"out") == 0) {
        info->type = gpio_type_out;
    } else if (strcmp(argv[1],"int") == 0) {
        info->type = gpio_type_int;
    } else {
        return -1;
    }

    info->num = atoi(argv[2]);
    info->out_val = atoi(argv[3]);

    return 0;
}

static void gpio_tools(rt_int32_t argc, char* argv[])
{
    gpio_test_info_t info;

    int r = gpio_parse_args(argc, argv, &info);
    if (r != 0) {
        print_usage();
        return;
    }

    switch (info.type) {
    case gpio_type_in:
        gpio_set_mode(info.num, gpio_mode_input);
        int status = gpio_get_value(info.num);
        rt_kprintf("gpio %u set to input, read status is %d\n", info.num, status);
        break;
    case gpio_type_out:
        gpio_set_mode(info.num, gpio_mode_output);
        gpio_set_value(info.num, info.out_val);
        rt_kprintf("gpio %u set to ouput %s, pls check voltage\n",
                info.num, info.out_val ? "high" : "low");
        break;
    case gpio_type_int:
        if (info.int_ops == GPIO_TOOL_SET_INT) {
            r = gpio_set_raw_int_mode(info.num, gpio_int_mode_rising);
            if (r != 0) {
                rt_kprintf("gpio_set_int_mode fail\n");
            }
            rt_kprintf("gpio %u set to rising edge interrupt mode\n", info.num);
        } else if (info.int_ops == GPIO_TOOL_GET_INT) {
            r = gpio_get_raw_int_status(info.num);
            if (r == 1) {
                gpio_clear_raw_int_status(info.num);
                rt_kprintf("gpio %u rising edge interrupt triggred and cleared\n", info.num);
            } else {
                rt_kprintf("gpio %u get no interrupt yet\n", info.num);
            }
        } else {
            rt_kprintf("inturrput operation %u error\n", info.int_ops);
        }
        break;
    default:
        print_usage();
    }
}

MSH_CMD_EXPORT(gpio_tools, in/out/int gpio_num [value] [-h] to get help);
