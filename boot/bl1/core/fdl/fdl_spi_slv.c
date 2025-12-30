/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "fdl_channel.h"
#include "cmn.h"
#include "dma.h"
#include "dw_spi.h"

struct FDL_ChannelHandler gSpiChannel;

static int spi_dl_open(FDL_ChannelHandler_T *channel)
{
	axi_dma_hw_init();
	spi_slv_hw_init();
	return 0;
}
static int spi_dl_read(FDL_ChannelHandler_T *channel, u8 *buf, u32 len, u32 timeout_ms)
{
	return spi_slv_read(len, buf, timeout_ms);
}
static int spi_dl_write(FDL_ChannelHandler_T *channel, const u8 *buf, u32 len, u32 timeout_ms)
{
	return spi_slv_write(len, (void *)buf, timeout_ms);
}

struct FDL_ChannelHandler gSpiChannel = {
	.channel = DL_CHAN_SPI,
	.open = spi_dl_open,
	.read = spi_dl_read,
	.write = spi_dl_write,
	.priv = 0,
};
