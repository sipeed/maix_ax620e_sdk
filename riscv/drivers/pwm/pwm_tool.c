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
#include <stdbool.h>
#include "drv_pwm.h"

typedef struct {
    pwm_e pwm;
    char *pwm_name;
} pwm_str_2_val_t;

pwm_str_2_val_t pwm_test_info[pwm_max] = {
    {.pwm = pwm_00, .pwm_name = "pwm00"},
    {.pwm = pwm_01, .pwm_name = "pwm01"},
    {.pwm = pwm_02, .pwm_name = "pwm02"},
    {.pwm = pwm_03, .pwm_name = "pwm03"},
    {.pwm = pwm_10, .pwm_name = "pwm10"},
    {.pwm = pwm_11, .pwm_name = "pwm11"},
    {.pwm = pwm_12, .pwm_name = "pwm12"},
    {.pwm = pwm_13, .pwm_name = "pwm13"},
    {.pwm = pwm_20, .pwm_name = "pwm20"},
    {.pwm = pwm_21, .pwm_name = "pwm21"},
    {.pwm = pwm_22, .pwm_name = "pwm22"},
    {.pwm = pwm_23, .pwm_name = "pwm23"}
};

static void print_usage()
{
    rt_kprintf("Usage: pwm freq duty enable\n"
               "    pwm:  pwm00 ~ pwm23\n"
               "    freq:  pwm frequency\n"
               "    duty:  pwm duty\n"
               "    enable: 0 disable 1 enable\n");
}

static void pwm_tool(int argc, char* argv[])
{
    pwm_e pwm = pwm_max;
    if(argc != 6) {
        print_usage();
        return;
    }

    char *pwm_name = argv[1];
    uint32_t freq_hz = atoi(argv[2]);
    uint8_t duty = atoi(argv[3]);
    bool en = atoi(argv[4]) ? true : false;
    for (int i = 0; i < sizeof(pwm_test_info) / sizeof(pwm_test_info[0]); i++) {
        if (rt_strcmp(pwm_test_info[i].pwm_name, pwm_name) == 0) {
            pwm = pwm_test_info[i].pwm;
        }
    }
    rt_kprintf("pwm %u for %u Hz %u%% duty\n", en ? "enable" : "disable" , pwm, freq_hz, duty);

    if (en) {
        int r = pwm_init(pwm, freq_hz, duty);
        if (r != 0) {
            rt_kprintf("pwm_init error\n");
            return;
        }
        r = pwm_start(pwm);
        if (r != 0) {
            rt_kprintf("pwm_start error\n");
        }
    } else {
        int r = pwm_stop(pwm);
        if (r != 0) {
            rt_kprintf("pwm_stop error\n");
        }
        r = pwm_deinit(pwm);
        if (r != 0) {
            rt_kprintf("pwm_deinit error\n");
            return;
        }
    }
}

MSH_CMD_EXPORT(pwm_tool, pwm freq duty enable [-h] to get help);
