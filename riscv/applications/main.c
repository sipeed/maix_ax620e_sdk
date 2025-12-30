/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#define AX_LOG_TAG

#include <rthw.h>
#include <rtthread.h>
#include <board.h>
#include <core_rv32.h>
#include "main.h"
#include "ax_log.h"

int main(void)
{
    AX_LOG_INFO("hello axera riscv");

    while (1) {
        rt_thread_mdelay(5000);
    }

    return 0;
}
