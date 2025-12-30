/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "chip_reg.h"
#include "dma.h"
#include "dwdma.h"


static u32 dma_hw_inited = 0;

static void axi_dma_reset()
{
	writel(DMAC_RST_MASK, DMAC_RESET);
	/* wait until DMAC reset done */
	while(readl(DMAC_RESET) & DMAC_RST_MASK);
}

static void axi_dma_enable()
{
	u32 val;
	val = readl(DMAC_CFG);
	val |= DMAC_EN_MASK;
	writel(val, DMAC_CFG);
}

static void axi_chan_enable(u32 ch)
{
	u32 val;
	val = readl(DMAC_CHEN_L);
	val |= BIT(ch-1) << DMAC_CHAN_EN_SHIFT |
		BIT(ch-1) << DMAC_CHAN_EN_WE_SHIFT;
	writel(val, DMAC_CHEN_L);
}

static void axi_chan_disable(u32 ch)
{
	u32 val;
	val = readl(DMAC_CHEN_L);
	val &= ~(BIT(ch-1) << DMAC_CHAN_EN_SHIFT);
	val |= BIT(ch-1) << DMAC_CHAN_EN_WE_SHIFT;
	writel(val, DMAC_CHEN_L);
}

static u32 axi_chan_is_hw_enable(u32 ch)
{
	u32 val;
	val = readl(DMAC_CHEN_L);
	return val & (BIT(ch-1) << DMAC_CHAN_EN_SHIFT);
}

static void axi_dma_irq_disable()
{
	u32 val;
	val = readl(DMAC_CFG);
	val &= ~INT_EN_MASK;
	writel(val, DMAC_CFG);
}

static void axi_chan_irq_set(u32 ch, u32 irq_mask)
{
	writel(irq_mask, DMAC_CH_INTSTATUS_ENA(ch));
}

static void axi_chan_irq_sig_set(u32 ch, u32 irq_mask)
{
	writel(irq_mask, DMAC_CH_INTSIGNAL_ENA(ch));
}

static void axi_chan_irq_clear(u32 ch, u32 irq_mask)
{
	writel(irq_mask, DMAC_CH_INTCLEAR(ch));
}

static u32 axi_chan_irq_read(u32 ch)
{
	return readl(DMAC_CH_INTSTATUS(ch));
}


static int axi_chan_config(u32 ch, unsigned long src_addr, unsigned long dst_addr, u32 xfer_len,
	u32 src_width, u32 dst_width, u32 burst_len,
	enum dma_xfer_direction direction, u32 per_req_num)
{
	u32 blk_ts, ctl_lo = 0, ctl_hi = 0, cfg_lo = 0, cfg_hi = 0;

	writel((u32)src_addr, DMAC_CH_SAR_L(ch));
	writel((u32)(src_addr >> 32), DMAC_CH_SAR_H(ch));

	writel((u32)dst_addr, DMAC_CH_DAR_L(ch));
	writel((u32)(dst_addr >> 32), DMAC_CH_DAR_H(ch));

	blk_ts = xfer_len >> src_width;
	if (blk_ts > DMAC_MAX_BLK_SIZE)
		return -1;
	writel((blk_ts-1), DMAC_CH_BLOCK_TS(ch));

	ctl_lo = burst_len << CH_CTL_L_DST_MSIZE_POS | burst_len << CH_CTL_L_SRC_MSIZE_POS |
		dst_width << CH_CTL_L_DST_WIDTH_POS | src_width << CH_CTL_L_SRC_WIDTH_POS;

	if (direction == DMA_MEM_TO_DEV) {
		ctl_lo |= (DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS |
			DWAXIDMAC_CH_CTL_L_NOINC << CH_CTL_L_DST_INC_POS);

		cfg_lo = per_req_num << CH_CFG_H_DST_PER_POS;
		cfg_hi = (DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC << CH_CFG_H_TT_FC_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_DST_POS);
	} else if (direction == DMA_DEV_TO_MEM) {
		ctl_lo |= (DWAXIDMAC_CH_CTL_L_NOINC << CH_CTL_L_SRC_INC_POS |
			DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS);
		cfg_lo = per_req_num << CH_CFG_H_SRC_PER_POS;
		cfg_hi = (DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC << CH_CFG_H_TT_FC_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_SRC_POS);
	} else { //DMA_MEM_TO_MEM
		ctl_lo |= (DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS |
			DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS);
		cfg_hi = (DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC << CH_CFG_H_TT_FC_POS);
	}

	ctl_hi &= ~CH_CTL_H_LLI_VALID;

	writel(ctl_lo, DMAC_CH_CTL_L(ch));
	writel(ctl_hi, DMAC_CH_CTL_H(ch));

	writel(cfg_lo, DMAC_CH_CFG_L(ch));
	writel(cfg_hi, DMAC_CH_CFG_H(ch));

	return 0;
}

/* DMA init function */
void axi_dma_hw_init()
{
	u32 ch;
	// u32 val;

	if (dma_hw_inited)
		return;
	axi_dma_reset();
	axi_dma_enable();
	axi_dma_irq_disable();

	for (ch = 1; ch <= DMAC_MAX_CHANNELS; ch++) {
		axi_chan_disable(ch);
		axi_chan_irq_clear(ch, DWAXIDMAC_IRQ_ALL);
		axi_chan_irq_sig_set(ch, DWAXIDMAC_IRQ_NONE);
		axi_chan_irq_set(ch, DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_ALL_ERR);
	}
	dma_hw_inited = 1;
}


/*
 * this function is only for async memset
 */
int axi_dma_xfer_memset(u32 ch, unsigned long src_addr, unsigned long dst_addr, u32 xfer_len,
		u32 src_width, u32 dst_width, u32 burst_len)
{
	u32 blk_ts, ctl_lo = 0, ctl_hi = 0, cfg_lo = 0, cfg_hi = 0;

	/* check if the channel is in use */
	if (axi_chan_is_hw_enable(ch))
		return -1;

	writel(src_addr, DMAC_CH_SAR_L(ch));
	writel(0, DMAC_CH_SAR_H(ch));
	writel(dst_addr, DMAC_CH_DAR_L(ch));
	writel(0, DMAC_CH_DAR_H(ch));

	//4B align, < 4B neet to do

	blk_ts = xfer_len >> src_width;
	if (blk_ts > DMAC_MAX_BLK_SIZE)
		return -1;
	writel((blk_ts-1), DMAC_CH_BLOCK_TS(ch));

	ctl_lo = burst_len << CH_CTL_L_DST_MSIZE_POS | burst_len << CH_CTL_L_SRC_MSIZE_POS |
		dst_width << CH_CTL_L_DST_WIDTH_POS | src_width << CH_CTL_L_SRC_WIDTH_POS;
	//DMA_MEM_TO_MEM
	ctl_lo |= (DWAXIDMAC_CH_CTL_L_NOINC << CH_CTL_L_SRC_INC_POS |
		DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS);
	cfg_hi = (DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC << CH_CFG_H_TT_FC_POS);
	ctl_hi &= ~CH_CTL_H_LLI_VALID;

	writel(ctl_lo, DMAC_CH_CTL_L(ch));
	writel(ctl_hi, DMAC_CH_CTL_H(ch));
	writel(cfg_lo, DMAC_CH_CFG_L(ch));
	writel(cfg_hi, DMAC_CH_CFG_H(ch));

	/* enable the channel to start transfer */
	axi_chan_enable(ch);
	return 0;
}


/**
 * @chan: the channel will transfer data, legal values:1~8
 * @src_addr: memory address or peripheral FIFO data port
 * @dst_addr: memory address or peripheral FIFO data port
 * @xfer_len: data size will be transferred
 * @src_width: legal values are 0x1(16 bits), 0x2(32 bit),
 * 0x3(64 bits), 0x4(128 bit), 0x5(256 bits), 0x6(512 bits)
 * @dst_width: same as src_addr_width but for destination
 * @burst_len: typically set as half the FIFO depth on I/O peripherals,
 *	legal values are 0x0(1 data item), 0x1(4 data item), 0x2(8 data item),
 *	0x3(16 data item), 0x4(32 data item), 0x5(64 data item), 0x6(128 data item)
 *	0x7(256 data item), 0x8(512 data item), 0x9(1024 data item)
 * @direction: legal values are DMA_MEM_TO_MEM/DMA_DEV_TO_MEM/DMA_MEM_TO_DEV
 * @handshake_if: the handshake interface with I/O peripherals
 */
int axi_dma_xfer_start(u32 ch, unsigned long src_addr, unsigned long dst_addr, u32 xfer_len,
		u32 src_width, u32 dst_width, u32 burst_len,
		enum dma_xfer_direction direction, u32 per_req_num)
{
	int ret;
	/* check if the channel is in use */
	if (axi_chan_is_hw_enable(ch)) {
		return -1;
	}

	/* configure the channel */
	ret = axi_chan_config(ch, src_addr, dst_addr, xfer_len,\
		src_width, dst_width, burst_len,\
		direction, per_req_num);
	if (ret < 0)
		return -1;

	/* enable the channel to start transfer */
	axi_chan_enable(ch);
	return 0;
}

int axi_dma_wait_xfer_done(u32 ch)
{
	u32 irq_mask, status;

	/* wait until DMA transfer done */
	irq_mask = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_ALL_ERR;
	do {
		status = axi_chan_irq_read(ch);
	} while ((status & irq_mask) == 0);

	/* clear int */
	axi_chan_irq_clear(ch, status);

	/* check if error occurred during transfer */
	if (status & DWAXIDMAC_IRQ_ALL_ERR) {
		axi_chan_disable(ch);
		return -1;
	}

	/* transfer done normally */
	axi_chan_disable(ch);
	return 0;
}

