#include "dmaper.h"

static ax_dma_per_lli_info_t lli_info[MAX_DMAPER_CHN];
static unsigned char req_id[MAX_DMAPER_CHN];

static void ax_dma_per_set_mask(u32 addr, u32 val, u32 shift, u32 mask)
{
	u32 tmp;
	tmp = ~(mask << shift) & readl(addr);
	writel((val << shift) | tmp, addr);
}

static void ax_dma_per_preset(void)
{
	writel(BIT(AX_DMAPER_PRST_SHIFT), AX_DMAPER_RST_SET);
	writel(BIT(AX_DMAPER_PRST_SHIFT), AX_DMAPER_RST_CLR);
}

static void ax_dma_per_areset(void)
{
	writel(BIT(AX_DMAPER_ARST_SHIFT), AX_DMAPER_RST_SET);
	writel(BIT(AX_DMAPER_ARST_SHIFT), AX_DMAPER_RST_CLR);
}

static void ax_dma_per_clk_en(char en)
{
	if (en) {
		writel(BIT(AX_DMAPER_PCLK_SHIFT), AX_DMAPER_PCLK_EB_SET);
		writel(BIT(AX_DMAPER_ACLK_SHIFT), AX_DMAPER_ACLK_EB_SET);
	} else {
		writel(BIT(AX_DMAPER_PCLK_SHIFT), AX_DMAPER_PCLK_EB_CLR);
		writel(BIT(AX_DMAPER_ACLK_SHIFT), AX_DMAPER_ACLK_EB_CLR);
	}
}

static void ax_dma_per_chn_en(u32 ch, u8 en)
{
	u32 val;
	if (en) {
		writel(BIT(ch), AX_DMAPER_CHN_EN);
	} else {
		val = readl(AX_DMAPER_CHN_EN);
		writel(~BIT(ch) & val, AX_DMAPER_CHN_EN);
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
		writel(BIT(per_req_num - 32), AX_DMA_REQ_SEL_H_SET);
	else
		writel(BIT(per_req_num), AX_DMA_REQ_SEL_L_SET);

	writel(GENMASK(5, 0) << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_CLR(ch / 5));
	writel(per_req_num << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_SET(ch / 5));
}

static void ax_dma_per_clr_req_id(u32 ch, u32 per_req_num)
{
	if (per_req_num > 31)
		writel(BIT(per_req_num - 32), AX_DMA_REQ_SEL_H_CLR);
	else
		writel(BIT(per_req_num), AX_DMA_REQ_SEL_H_CLR);

	writel(GENMASK(5, 0) << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_SET(ch / 5));
}

static void ax_dma_per_set_lli(u32 ch, unsigned long src_addr, unsigned long dst_addr, u32 xfer_len,
			       u32 src_width, u32 dst_width, u32 burst_len,
			       u32 endian, enum dma_xfer_direction direction)
{
	u32 burst_size_byte;

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
		burst_size_byte = burst_len << lli_info[ch].ctrl.src_tr_wridth;
		lli_info[ch].ctrl.dst_tr_wridth = ffs(xfer_len | 0x8 | burst_size_byte) - 1;
		lli_info[ch].ctrl.src_msize = burst_len;
		lli_info[ch].ctrl.dst_msize = burst_size_byte >> lli_info[ch].ctrl.dst_tr_wridth;
	} else {
		lli_info[ch].ctrl.src_per = 0;
		lli_info[ch].ctrl.sinc = 1;
		lli_info[ch].ctrl.dinc = 0;
		lli_info[ch].ctrl.arlen = DMAC_TRANS_WIDTH_32;
		lli_info[ch].ctrl.awlen = DMAC_TRANS_WIDTH_16;
		lli_info[ch].ctrl.dst_tr_wridth = dst_width;
		//Dynamic change tr width for mem
		burst_size_byte = burst_len << lli_info[ch].ctrl.dst_tr_wridth;
		lli_info[ch].ctrl.src_tr_wridth = ffs(xfer_len | 0x8 | burst_size_byte) - 1;
		lli_info[ch].ctrl.dst_msize = burst_len;
		lli_info[ch].ctrl.src_msize = burst_size_byte >> lli_info[ch].ctrl.src_tr_wridth;
	}
	lli_info[ch].data.block_ts = xfer_len >> lli_info[ch].ctrl.src_tr_wridth;
	writel(((u64)(unsigned long)&lli_info[ch]) >> 32, AX_DMAPER_CHN_LLI_H(ch));
	writel(((u64)(unsigned long)&lli_info[ch]) & 0xFFFFFFFF, AX_DMAPER_CHN_LLI_L(ch));
}

void axi_dma_hw_init()
{
	int i;

	ax_dma_per_clk_en(1);
	ax_dma_per_preset();
	ax_dma_per_areset();
	//disabled lli prefetch
	ax_dma_per_lli_prefetch(0);
	//set default req id (0x3f)to all chan, conflict prevention
	for (i = 0; i < 4; i++)
		writel(0xffffffff, AX_DMA_REQ_HS_SEL_L_SET(i));
}

int axi_dma_xfer_start(u32 ch, unsigned long src_addr, unsigned long dst_addr, u32 xfer_len,
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

	writel(BIT(ch) | INT_RESP_ERR, AX_DMAPER_INT_GLB_MASK);
	writel(RESP_ALL_ERR, AX_DMAPER_INT_RESP_ERR_MASK);
	writel(CHN_TRANSF_DONE, AX_DMAPER_INT_MASK(ch));

	writel(BIT(ch), AX_DMAPER_START);

	return 0;
}

int axi_dma_wait_xfer_done(u32 ch)
{
	/* wait until DMA transfer done */
	while (!readl(AX_DMAPER_INT_STA(ch))) ;

	/* clear int */
	writel(CHN_TRANSF_DONE, AX_DMAPER_INT_CLR(ch));

	ax_dma_per_clr_req_id(ch, req_id[ch]);

	ax_dma_per_chn_en(ch, 0);
	return 0;
}

void ax_dma_per_suspend(u32 ch)
{
	ax_dma_per_set_mask(AX_DMAPER_CTRL, 1, ch, 1);
}

void ax_dma_per_resume(u32 ch)
{
	while (!(readl(AX_DMAPER_STA) & BIT(ch))) ;
	ax_dma_per_set_mask(AX_DMAPER_CTRL, 0, ch, 1);
}
