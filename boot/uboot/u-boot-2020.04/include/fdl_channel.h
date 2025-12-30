/*
 * (C) Copyright 2020 AXERA
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __FDL_CHANNEL_H__
#define __FDL_CHANNEL_H__
#include <common.h>
#include <asm/arch/boot_mode.h>

typedef struct FDL_ChannelHandler {
	dl_channel_t channel;
	int (*open) (struct FDL_ChannelHandler * channel);
	int (*read) (struct FDL_ChannelHandler * channel, u8 * buf, u32 len);
	 u8(*getchar) (struct FDL_ChannelHandler * channel);
	 u32(*write) (struct FDL_ChannelHandler * channel, const u8 * buf, u32 len);
	int (*putchar) (struct FDL_ChannelHandler * channel, const u8 ch);
	int (*setbaudrate) (struct FDL_ChannelHandler * channel, u32 baudrate);
	void *priv;
} FDL_ChannelHandler_T;

int usb_handshake(u32 timeout_ms);
int uart_handshake(u32 timeout_ms);
int fdl_channel_init(void);
#endif
