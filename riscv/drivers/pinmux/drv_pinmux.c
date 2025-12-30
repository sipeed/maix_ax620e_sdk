/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <rtdevice.h>
#include "ax_common.h"

static reg_info_t pinmux_info[] = {
    /* GPIO for boot up debug */
    {0x00060083, 0x02304084},  /* PadName = UART3_TXD            Fuction = GPIO1_A2 */

    /* UART0/UART1 */
    {0x00000083, 0x0230403C},  /* PadName = UART0_TXD            Fuction = UART0_TXD */
    {0x00000083, 0x02304048},  /* PadName = UART0_RXD            Fuction = UART0_RXD */
    {0x00000083, 0x02304054},  /* PadName = UART1_TXD            Fuction = UART1_TXD */
#ifdef AX_BOARD_MINION_SUPPORT
    {0x00010003, 0x02304060},  /* PadName = UART1_RXD            Fuction = MCLK7 */
#else
    {0x00000083, 0x02304060},  /* PadName = UART1_RXD            Fuction = UART1_RXD */
#endif
    /* I2C0/I2C1/I2C2 */
    {0x00000083, 0x0230400C},  /* PadName = I2C0_SCL             Fuction = I2C0_SCL */
    {0x00000083, 0x02304018},  /* PadName = I2C0_SDA             Fuction = I2C0_SDA */
    {0x00000083, 0x02304024},  /* PadName = I2C1_SCL             Fuction = I2C1_SCL */
    {0x00000083, 0x02304030},  /* PadName = I2C1_SDA             Fuction = I2C1_SDA */
#if defined(CHIP_AX620Q) && defined(AX620E_NAND)
    {0x00030083, 0x02300018},  /* PadName = VI_D1                Fuction = I2C6_SCL */
    {0x00030083, 0x0230000C},  /* PadName = VI_D0                Fuction = I2C6_SDA */
    /* RESET SNS1/SNS2 */
    {0x00060003, 0x0230003C},  /* PadName = VI_D4                Fuction = GPIO0_A4 */
    {0x00060003, 0x02300084},  /* PadName = VI_CLK0              Fuction = GPIO0_A10 */
#endif
#if defined(CHIP_AX630C)
    {0x00020083, 0x104F2024},  /* PadName = SDIO_CLK             Fuction = I2C2_SCL */
    {0x00010083, 0x104F2030},  /* PadName = SDIO_CMD             Fuction = I2C2_SDA */
#endif
    /* SENSOR RST0/RST1 */
    {0x000000C3, 0x02302018},  /* PadName = GPIO3_A1             Fuction = GPIO3_A1 */
#if defined(CHIP_AX630C)
    {0x00060083, 0x104F000C},  /* PadName = EMAC_PTP_PPS0        Fuction = GPIO1_A8 */
#endif
};

static int pinmux_init(void)
{
    for (int i = 0; i < sizeof(pinmux_info) / sizeof(pinmux_info[0]); i++) {
        ax_writel(pinmux_info[i].reg_val, pinmux_info[i].reg_addr);
    }
    return 0;
}

INIT_BOARD_EXPORT(pinmux_init);
