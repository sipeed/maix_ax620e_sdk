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
#include "dmaper.h"
#include "ax_log.h"
#include "mc20e_e907_interrupt.h"
#include "ax_common.h"

#define DMA_TRANSFER_TIMEOUT_MS 100

//static ax_dma_per_lli_info_t *lli_info = 0x40000000;
//static ax_dma_per_lli_info_t lli_info[MAX_DMAPER_CHN];
static ax_dma_per_lli_info_t *lli_info = (ax_dma_per_lli_info_t *)0xa00;
static unsigned char req_id[MAX_DMAPER_CHN];

static void ax_dma_per_set_mask(u32 addr, u32 val, u32 shift, u32 mask)
{
	u32 tmp;
	tmp = ~(mask << shift) & ax_readl(addr);
	ax_writel((val << shift) | tmp, addr);
}

static void ax_dma_per_chn_en(u32 ch, u8 en)
{
	u32 val;
	if (en) {
		ax_writel(BIT(ch), AX_DMAPER_CHN_EN);
	} else {
		val = ax_readl(AX_DMAPER_CHN_EN);
		ax_writel(~BIT(ch) & val, AX_DMAPER_CHN_EN);
	}
}

static void ax_dma_per_lli_prefetch(u8 en)
{
	ax_dma_per_set_mask(AX_DMAPER_CTRL, en, LLI_RD_PREFETCH_SHIFT,
			    LLI_RD_PREFETCH_MASK);
}

static void ax_dma_per_set_req_id(u32 ch, u32 per_req_num)
{
	if (per_req_num > 31)
		ax_writel(BIT(per_req_num - 32), AX_DMA_REQ_SEL_H_SET);
	else
		ax_writel(BIT(per_req_num), AX_DMA_REQ_SEL_L_SET);

	ax_writel(GENMASK(5, 0) << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_CLR(ch / 5));
	ax_writel(per_req_num << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_SET(ch / 5));
}

static void ax_dma_per_clr_req_id(u32 ch, u32 per_req_num)
{
	if (per_req_num > 31)
		ax_writel(BIT(per_req_num - 32), AX_DMA_REQ_SEL_H_CLR);
	else
		ax_writel(BIT(per_req_num), AX_DMA_REQ_SEL_H_CLR);

	ax_writel(GENMASK(5, 0) << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_SET(ch / 5));
}

static void ax_dma_per_set_lli(u32 ch, u64 src_addr, u64 dst_addr, u32 xfer_len,
			       u32 src_width, u32 dst_width, u32 burst_len,
			       u32 endian, enum dma_xfer_direction direction)
{
	lli_info[ch].sar = src_addr;
	lli_info[ch].dar = dst_addr;

	lli_info[ch].ctrl.last = 1;
	lli_info[ch].ctrl.ioc = 0;
	lli_info[ch].ctrl.endian = endian;
	lli_info[ch].ctrl.dst_osd = 0xf;
	lli_info[ch].ctrl.src_osd = 0xf;
	lli_info[ch].data.llp = 0;

	if (direction == DMA_DEV_TO_MEM) {
		lli_info[ch].ctrl.src_per = 1;
		lli_info[ch].ctrl.sinc = 0;
		lli_info[ch].ctrl.dinc = 1;
		lli_info[ch].ctrl.arlen = DMAC_TRANS_WIDTH_16;
		lli_info[ch].ctrl.awlen = DMAC_TRANS_WIDTH_32;
		lli_info[ch].ctrl.src_tr_wridth = src_width;
		lli_info[ch].ctrl.dst_tr_wridth = __rt_ffs(xfer_len | 0x8) - 1;
	} else {
		lli_info[ch].ctrl.src_per = 0;
		lli_info[ch].ctrl.sinc = 1;
		lli_info[ch].ctrl.dinc = 0;
		lli_info[ch].ctrl.arlen = DMAC_TRANS_WIDTH_32;
		lli_info[ch].ctrl.awlen = DMAC_TRANS_WIDTH_16;
		lli_info[ch].ctrl.dst_tr_wridth = dst_width;
		lli_info[ch].ctrl.src_tr_wridth = __rt_ffs(xfer_len | 0x8) - 1;
	}
	lli_info[ch].ctrl.src_msize = burst_len;
	lli_info[ch].ctrl.dst_msize = burst_len;
	lli_info[ch].data.block_ts = xfer_len >> lli_info[ch].ctrl.src_tr_wridth;

	csi_dcache_clean_range((uint32_t *)&lli_info[ch], sizeof(lli_info[ch]));
	csi_dcache_invalid_range((uint32_t *)&lli_info[ch], sizeof(lli_info[ch]));

	ax_writel(0, AX_DMAPER_CHN_LLI_H(ch));
	ax_writel((u32)&lli_info[ch], AX_DMAPER_CHN_LLI_L(ch));
}

void axi_dma_hw_init()
{
	int i;

	//disabled lli prefetch
	ax_dma_per_lli_prefetch(0);
	//set default req id (0x3f)to all chan, conflict prevention
	for (i = 0; i < 4; i++)
		ax_writel(0xffffffff, AX_DMA_REQ_HS_SEL_L_SET(i));
}

static rt_sem_t dmaper_sem;
static int axi_dma_sem_init()
{
	rt_sem_t sem = rt_sem_create("dmaper_sem", 0, RT_IPC_FLAG_PRIO);
	if (sem == RT_NULL) {
		AX_LOG_ERROR("create dmaper sem fail");
		return -1;
	}
	dmaper_sem = sem;

	return 0;
}

int axi_dma_xfer_start(u32 ch, u64 src_addr, u64 dst_addr, u32 xfer_len,
		       u32 src_width, u32 dst_width, u32 burst_len, u32 endian,
		       enum dma_xfer_direction direction, u32 per_req_num)
{
	if (ch > MAX_DMAPER_CHN - 1)
		return -1;
	req_id[ch] = per_req_num;
	ax_dma_per_set_req_id(ch, per_req_num);
	ax_dma_per_set_lli(ch, src_addr, dst_addr, xfer_len, src_width,
			   dst_width, burst_len, endian, direction);
	ax_dma_per_chn_en(ch, 1);

	ax_writel(BIT(ch) | INT_RESP_ERR, AX_DMAPER_INT_GLB_MASK);
	ax_writel(RESP_ALL_ERR, AX_DMAPER_INT_RESP_ERR_MASK);
	ax_writel(CHN_TRANSF_DONE, AX_DMAPER_INT_MASK(ch));

	ax_writel(BIT(ch), AX_DMAPER_START);

	return 0;
}

static void ax_dmaper_dump(u32 ch)
{
	int i;

	AX_LOG_ERROR("ax_dmaper_dump start channel %d", ch);
	for (i = 0; i < 0x20; i += 0x10) {
		AX_LOG_ERROR("dmaper reg: 0x%03x: %08x %08x %08x %08x", i,
			ax_readl(AX_DMAPER_BASE + i + 0x0), ax_readl(AX_DMAPER_BASE + i + 0x4),
			ax_readl(AX_DMAPER_BASE + i + 0x8), ax_readl(AX_DMAPER_BASE + i + 0xC));
	}
	AX_LOG_ERROR("dmaper lli base 0x%x(read: 0x%08x%08x) dump:",
			(u32)&lli_info[ch],
			ax_readl(AX_DMAPER_CHN_LLI_H(ch)), ax_readl(AX_DMAPER_CHN_LLI_L(ch)));
	for (i = 0; i < 8; i += 4) {
		AX_LOG_ERROR("0x%08x 0x%08x 0x%08x 0x%08x",
			*((u32 *)&lli_info[ch] + i + 0), *((u32 *)&lli_info[ch] + i + 1),
			*((u32 *)&lli_info[ch] + i + 2), *((u32 *)&lli_info[ch] + i + 3));
	}
	AX_LOG_ERROR("dmaper src: 0x%08x%08x dst: 0x%08x%08x",
			ax_readl(AX_DMAPER_SRC_ADDR_H(ch)), ax_readl(AX_DMAPER_SRC_ADDR_L(ch)),
			ax_readl(AX_DMAPER_DST_ADDR_H(ch)), ax_readl(AX_DMAPER_DST_ADDR_L(ch)));
	AX_LOG_ERROR("dmaper pclk(%s) aclk(%s) prst(%s) arst(%s)",
			(ax_readl(AX_DMAPER_PCLK_EB) & BIT(AX_DMAPER_PCLK_SHIFT)) ? "on" : "off",
			(ax_readl(AX_DMAPER_ACLK_EB) & BIT(AX_DMAPER_ACLK_SHIFT)) ? "on" : "off",
			(ax_readl(AX_DMAPER_RST) & BIT(AX_DMAPER_PRST_SHIFT)) ? "set" : "release",
			(ax_readl(AX_DMAPER_RST) & BIT(AX_DMAPER_ARST_SHIFT)) ? "set" : "release");
	AX_LOG_ERROR("ax_dmaper_dump end");
}

int axi_dma_wait_xfer_done(u32 ch)
{
	int ret = 0;
#if 0
	/* wait until DMA transfer done */
	while (!ax_readl(AX_DMAPER_INT_STA(ch))) ;

	/* clear int */
	ax_writel(CHN_TRANSF_DONE, AX_DMAPER_INT_CLR(ch));

	ax_dma_per_clr_req_id(ch, req_id[ch]);

	ax_dma_per_chn_en(ch, 0);
#else
	/* wait until DMA transfer done */
	ret = rt_sem_take(dmaper_sem, DMA_TRANSFER_TIMEOUT_MS);
	if (ret != RT_EOK) {
		ax_dmaper_dump(ch);
	}
#endif

	return ret;
}

static void axi_dma_irq_handler(int irqno, void *param)
{
	/* clear int */
	u32 status = ax_readl(AX_DMAPER_INT_GLB_STA);
	for (int ch = 0; ch < DMAC_CHAN_MAX; ch++) {
		if (status & (1 << ch)) {
			ax_writel(CHN_TRANSF_DONE, AX_DMAPER_INT_CLR(ch));
			ax_dma_per_clr_req_id(ch, req_id[ch]);
			ax_dma_per_chn_en(ch, 0);
			break;
		}
	}

	rt_sem_release(dmaper_sem);
}

int axi_dma_per_int_init()
{
	axi_dma_sem_init();
	mc20e_e907_interrupt_install(INT_REQ_AXERA_DMA_PER, axi_dma_irq_handler, NULL, "axi_dmaper_int");
	mc20e_e907_interrupt_umask(INT_REQ_AXERA_DMA_PER);

	return 0;
}

int axi_dma_per_int_deinit()
{
	mc20e_e907_interrupt_mask(INT_REQ_AXERA_DMA_PER);
	rt_sem_delete(dmaper_sem);

	return 0;
}

void ax_dma_per_suspend(u32 ch)
{
	ax_dma_per_set_mask(AX_DMAPER_CTRL, 1, ch, 1);
}

void ax_dma_per_resume(u32 ch)
{
	while (!(ax_readl(AX_DMAPER_STA) & BIT(ch))) ;
	ax_dma_per_set_mask(AX_DMAPER_CTRL, 0, ch, 1);
}
