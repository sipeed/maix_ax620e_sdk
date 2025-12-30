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
#include <rtdevice.h>
#include <stdlib.h>
#include "drv_timer64.h"

static void print_usage()
{
    rt_kprintf("Usage: test t64 delay n us/ms\n"
               "    delay unit: us or ms\n"
               "    delay duration: delay n us or ms\n");
}

static void t64_test(int argc, char* argv[])
{
    if(argc != 3) {
        print_usage();
        return;
    }

    char *delay_uint = argv[1];
    uint32_t delay = atoi(argv[2]);

    rt_kprintf("timer64 will count %u %s, pls check\n", delay, delay_uint);
    if (rt_strcmp(delay_uint, "us") == 0) {
        t64_udelay(delay);
    } else if (rt_strcmp(delay_uint, "ms") == 0) {
        t64_mdelay(delay);
    } else {
        rt_kprintf("delay unit %s error", delay_uint);
    }
    rt_kprintf("timer64 count %u %s end\n", delay, delay_uint);
}

MSH_CMD_EXPORT(t64_test, delay_us [-h] to get help);
