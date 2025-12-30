/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DRV_UART_H__
#define __DRV_UART_H__

#include "soc.h"
#include "mc20e_e907_interrupt.h"

typedef enum {
    uart_0 = 0,
    uart_1,
    uart_2,
    uart_3,
    uart_4,
    uart_5,
    uart_max
} uart_e;

typedef enum {
    uart_baudrate_9600 = 9600,
    uart_baudrate_19200 = 19200,
    uart_baudrate_38400 = 38400,
    uart_baudrate_57600 = 57600,
    uart_baudrate_115200 = 115200,
    uart_baudrate_921600 = 921600
} uart_baudrate_e;

struct ax_uart_info {
    uart_e num;
    char *name;
    uart_baudrate_e baudrate;
    bool status;
};

#endif // __DRV_UART_H__
