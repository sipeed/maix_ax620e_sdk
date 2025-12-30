/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __FDL_CHANNEL_H
#define __FDL_CHANNEL_H

#include "cmn.h"

#define UART_RDY            0x56
#define HOST_RDY            0x3C

struct uart_info{
	u32 reg_base;
	u32 baudrate;
};

typedef enum {
	DL_CHAN_UNKNOWN   = 0x0,
	DL_CHAN_SDIO      = 0x1,
	DL_CHAN_SPI       = 0x2,
	DL_CHAN_USB       = 0x3,
	DL_CHAN_UART      = 0x4, //0x4 is uart0, 0x5 is uart1
	DL_CHAN_UART1     = 0x5,
	DL_CHAN_SD        = 0x6,
} dl_channel_e;

typedef struct FDL_ChannelHandler {
	dl_channel_e channel;
	int (*open) (struct FDL_ChannelHandler *channel);
	char (*getchar) (struct FDL_ChannelHandler *channel, u32 timeout_ms);
	int (*putchar) (struct FDL_ChannelHandler *channel, const u8 ch, u32 timeout_ms);
	int (*read) (struct FDL_ChannelHandler *channel, u8 *buf, u32 len, u32 timeout_ms);
	int (*write) (struct FDL_ChannelHandler *channel, const u8 *buf, u32 len, u32 timeout_ms);
	int (*set_baudrate) (struct FDL_ChannelHandler *channel, u32 baud_rate);
	void *priv;
} FDL_ChannelHandler_T;

void uart_ready();
int fdl_channel_init(dl_channel_e chan, u32 timeout_ms);
#endif
