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
#include "drv_sw_int.h"

typedef struct {
    sw_int_group_e group;
    sw_int_channel_e channel;
} sw_int_test_info_t;

static sw_int_test_info_t test_info;

void sw_int_test_cb(void *param)
{
    sw_int_test_info_t *info = param;
    rt_kprintf("get group %u channel %u interrupt\n", info->group, info->channel);
}

static void print_usage()
{
    rt_kprintf("Usage: group channel enable\n"
               "    group:  0/1/2/3\n"
               "    channel:  0 ~ 31\n"
               "    register: 0 unregister 1 register\n");
}

static void sw_int_tool(int argc, char* argv[])
{
    if(argc != 4) {
        print_usage();
        return;
    }

    test_info.group = atoi(argv[1]);
    test_info.channel = atoi(argv[2]);
    bool en = atoi(argv[3]) ? true : false;

    rt_kprintf("group %u channel %u %s\n", test_info.group, test_info.channel, en ? "register": "unregister");

    if (en == true) {
        sw_int_cb_register(test_info.group, test_info.channel, sw_int_test_cb, &test_info);
    } else {
        sw_int_cb_unregister(test_info.group, test_info.channel);
    }
}

MSH_CMD_EXPORT(sw_int_tool, group channel sw_int_register/sw_int_unregister [-h] to get help);
