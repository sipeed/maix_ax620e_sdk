/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _DWDMA_EXT_H_
#define _DWDMA_EXT_H_
#include "cmn.h"

enum dma_xfer_direction{
	DMA_MEM_TO_MEM = 0,
	DMA_MEM_TO_DEV,
	DMA_DEV_TO_MEM,
};

enum {
	DMAC_TRANS_WIDTH_8		= 0,
	DMAC_TRANS_WIDTH_16,
	DMAC_TRANS_WIDTH_32,
	DMAC_TRANS_WIDTH_64,
	DMAC_TRANS_WIDTH_128,
	DMAC_TRANS_WIDTH_256,
	DMAC_TRANS_WIDTH_512,
	DMAC_TRANS_WIDTH_MAX	= DMAC_TRANS_WIDTH_512
};

enum {
	DMAC_BURST_TRANS_LEN_1	= 0,
	DMAC_BURST_TRANS_LEN_4,
	DMAC_BURST_TRANS_LEN_8,
	DMAC_BURST_TRANS_LEN_16,
	DMAC_BURST_TRANS_LEN_32,
	DMAC_BURST_TRANS_LEN_64,
	DMAC_BURST_TRANS_LEN_128,
	DMAC_BURST_TRANS_LEN_256,
	DMAC_BURST_TRANS_LEN_512,
	DMAC_BURST_TRANS_LEN_1024
};

/* dma channel mapping */
enum {
	DMAC_CHAN0	= 1,
	DMAC_CHAN_MAX  = DMAC_CHAN0,
};

/* peripheral dma request interface */
#define	SSI_DMA_RX_REQ		(1)
#define	SSI_DMA_TX_REQ		(0)
#define	SSI_S_DMA_RX_REQ	(3)
#define	SSI_S_DMA_TX_REQ	(2)

#define	UART0_TX_REQ		(40)
#define	UART0_RX_REQ		(41)
#define	UART1_TX_REQ		(42)
#define	UART1_RX_REQ		(43)

void axi_dma_hw_init();
int axi_dma_xfer_start(u32 ch, unsigned long src_addr, unsigned long dst_addr, u32 xfer_len,
		u32 src_width, u32 dst_width, u32 burst_len,
		enum dma_xfer_direction direction, u32 per_req_num);
int axi_dma_xfer_memset(u32 ch, unsigned long src_addr, unsigned long dst_addr, u32 xfer_len,
		u32 src_width, u32 dst_width, u32 burst_len);
int axi_dma_wait_xfer_done(u32 ch);

#endif
