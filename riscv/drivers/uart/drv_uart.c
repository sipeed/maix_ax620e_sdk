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
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "drv_uart.h"
#include "drv_uart_reg.h"
#include "ax_common.h"
#include "ax_log.h"

#define DISABLE (0)
#define ENABLE  (1)

static struct ax_uart_info ax_uart[uart_max] = {
    {uart_0, "uart0", uart_baudrate_921600, DISABLE},
    {uart_1, "uart1", uart_baudrate_921600, ENABLE},
    {uart_2, "uart2", uart_baudrate_921600, DISABLE},
    {uart_3, "uart3", uart_baudrate_921600, DISABLE},
    {uart_4, "uart4", uart_baudrate_921600, DISABLE},
    {uart_5, "uart5", uart_baudrate_921600, DISABLE},
};

static struct CSKY_UART_REG *uart_base_addr[uart_max] = {
    (struct CSKY_UART_REG *)UART0_BASE_ADDR,
    (struct CSKY_UART_REG *)UART1_BASE_ADDR,
    (struct CSKY_UART_REG *)UART2_BASE_ADDR,
    (struct CSKY_UART_REG *)UART3_BASE_ADDR,
    (struct CSKY_UART_REG *)UART4_BASE_ADDR,
    (struct CSKY_UART_REG *)UART5_BASE_ADDR
};

static uint32_t uart_irq_no[uart_max] = {
    INT_REQ_PERIPH_UART0,
    INT_REQ_PERIPH_UART1,
    INT_REQ_PERIPH_UART2,
    INT_REQ_PERIPH_UART3,
    INT_REQ_PERIPH_UART4,
    INT_REQ_PERIPH_UART5
};

static reg_info_t uart_pclk_info[uart_max] = {
    {1 << BIT_13, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_SET},
    {1 << BIT_14, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_SET},
    {1 << BIT_15, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_SET},
    {1 << BIT_16, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_SET},
    {1 << BIT_17, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_SET},
    {1 << BIT_18, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_3_SET}
};

static reg_info_t uart_clk_info[uart_max] = {
    {1 << BIT_5, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_6, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_7, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_8, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_9, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET},
    {1 << BIT_10, PERIPH_SYS_GLB_ADDR + PERIPH_CLK_EB_2_SET}
};

static struct csky_uart_dev uart_dev[uart_max];

static void uart_clk_init(uart_e uart)
{
    ax_writel(uart_pclk_info[uart].reg_val, uart_pclk_info[uart].reg_addr);
    ax_writel(uart_clk_info[uart].reg_val, uart_clk_info[uart].reg_addr);
}

void csky_uart_isr(int irqno, void *param)
{
    struct csky_uart_dev *uart = CSKY_UART_DEVICE(param);
    RT_ASSERT(uart != RT_NULL);

    /* read interrupt status and clear it */
    if ((uart->addr->IIR & DW_IIR_RECV_DATA) ||
        (uart->addr->IIR & DW_IIR_RECV_LINE)) {
        rt_hw_serial_isr(&uart->parent, RT_SERIAL_EVENT_RX_IND);
    }
}

/*
 * UART interface
 */
static rt_err_t csky_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    rt_err_t ret = RT_EOK;
    struct csky_uart_dev *uart;
    uart_baudrate_e baudrate;
    RT_ASSERT(serial != RT_NULL);

    serial->config = *cfg;
    baudrate = serial->config.baud_rate;

    uart = CSKY_UART_DEVICE(serial->parent.user_data);
    RT_ASSERT(uart != RT_NULL);

    uart->addr->IER = 0;
    uart->addr->MCR = 0x03; //CPR 0xf4
    uart->addr->FCR = 0x07;
    uart->addr->LCR = 0x03; // 8bits 1 stop_bit, NONE_PARITY
    uint32_t baud = (UART_CONSOLE_BASE_CLK + baudrate / 2) / 16 / baudrate;
    uint32_t lcr = uart->addr->LCR & ~0x80;
    uart->addr->LCR = lcr | 0x80;
    uart->addr->DLL = baud&0xFF;
    uart->addr->DLH = (baud>>8)&0xFF;
    uart->addr->LCR = lcr;
    uart->addr->IER = IER_RDA_INT_ENABLE | IIR_RECV_LINE_ENABLE;

    return ret;
}

static rt_err_t csky_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    rt_err_t ret = RT_EOK;
    struct csky_uart_dev *uart = CSKY_UART_DEVICE(serial->parent.user_data);
    RT_ASSERT(uart != RT_NULL);

    switch (cmd) {
    case RT_DEVICE_CTRL_CLR_INT:
        /* Disable the UART Interrupt */
        mc20e_e907_interrupt_mask(uart->irqno);
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* install interrupt */
        /* Enable the UART Interrupt */
        mc20e_e907_interrupt_umask(uart->irqno);
        break;

    case RT_DEVICE_CTRL_CONFIG:
        break;
    }

    return ret;
}

static int csky_uart_putc(struct rt_serial_device *serial, char c)
{
    struct csky_uart_dev *uart = CSKY_UART_DEVICE(serial->parent.user_data);

    RT_ASSERT(uart != RT_NULL);

    /* FIFO status, contain valid data */
    while ((!(uart->addr->LSR & DW_LSR_TRANS_EMPTY))) ;
    uart->addr->THR = c;

    return 1;
}

static int csky_uart_getc(struct rt_serial_device *serial)
{
    int ch = -1;
    struct csky_uart_dev *uart = CSKY_UART_DEVICE(serial->parent.user_data);

    RT_ASSERT(uart != RT_NULL);

    if (uart->addr->LSR & LSR_DATA_READY) {
        ch = uart->addr->RBR & 0xff;
    }

    return ch;
}

const static struct rt_uart_ops _uart_ops = {
    csky_uart_configure,
    csky_uart_control,
    csky_uart_putc,
    csky_uart_getc,
    RT_NULL,
};

/*
 * UART Initiation
 */
static int rt_hw_uart_init(void)
{
    rt_err_t ret = RT_EOK;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    int i;

    ax_log_init();

    for (i = 0; i < uart_max; i++) {
        if (uart_channel == ax_uart[i].num) {
            ax_uart[i].status = ENABLE;
            ax_uart[i].baudrate = uart_baudrate;
        }
        if (ax_uart[i].status) {

            uart_clk_init(ax_uart[i].num);
            config.baud_rate = ax_uart[i].baudrate;
            uart_dev[i].parent.ops = &_uart_ops;
            uart_dev[i].parent.config = config;
            uart_dev[i].irqno = uart_irq_no[ax_uart[i].num] ;
            uart_dev[i].addr  = uart_base_addr[ax_uart[i].num];

            ret = rt_hw_serial_register(&uart_dev[i].parent,
                                        ax_uart[i].name,
                                        RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                                        &uart_dev[i]);

            mc20e_e907_interrupt_install(uart_dev[i].irqno,
                                         csky_uart_isr,
                                         &(uart_dev[i].parent),
                                         uart_dev[i].parent.parent.parent.name);
        }
    }
    return ret;
}

INIT_BOARD_EXPORT(rt_hw_uart_init);
