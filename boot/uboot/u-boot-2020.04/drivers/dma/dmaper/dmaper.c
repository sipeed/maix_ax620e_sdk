#include "dmaper.h"

static unsigned char req_id[MAX_DMAPER_CHN] = {0};
static char* lli_base_ptr[MAX_DMAPER_CHN] = {0};

#define ax_writel(x, a) writel(x, ((unsigned int *)(unsigned long)a))
#define ax_readl(a) readl(((unsigned int *)(unsigned long)a))

static void ax_dma_per_set_mask(u32 addr, u32 val, u32 shift, u32 mask)
{
	u32 tmp;
	tmp = ~(mask << shift) & ax_readl(addr);
	ax_writel((val << shift) | tmp, addr);
}

static void ax_dma_per_preset(void)
{
	ax_writel(BIT(AX_DMAPER_PRST_SHIFT), AX_DMAPER_RST_SET);
	ax_writel(BIT(AX_DMAPER_PRST_SHIFT), AX_DMAPER_RST_CLR);
}

static void ax_dma_per_areset(void)
{
	ax_writel(BIT(AX_DMAPER_ARST_SHIFT), AX_DMAPER_RST_SET);
	ax_writel(BIT(AX_DMAPER_ARST_SHIFT), AX_DMAPER_RST_CLR);
}

static void ax_dma_per_clk_en(char en)
{
	if (en) {
		ax_writel(BIT(AX_DMAPER_PCLK_SHIFT), AX_DMAPER_PCLK_EB_SET);
		ax_writel(BIT(AX_DMAPER_ACLK_SHIFT), AX_DMAPER_ACLK_EB_SET);
	} else {
		ax_writel(BIT(AX_DMAPER_PCLK_SHIFT), AX_DMAPER_PCLK_EB_CLR);
		ax_writel(BIT(AX_DMAPER_ACLK_SHIFT), AX_DMAPER_ACLK_EB_CLR);
	}
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
	//set axera dma req chail
	if (per_req_num > 31)
		ax_writel(BIT(per_req_num - 32), AX_DMA_REQ_SEL_H_SET);
	else
		ax_writel(BIT(per_req_num), AX_DMA_REQ_SEL_L_SET);

	ax_writel(GENMASK(5, 0) << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_CLR(ch / 5));
	ax_writel(per_req_num << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_SET(ch / 5));
}

static void ax_dma_per_clr_req_id(u32 ch, u32 per_req_num)
{
	//clr axera dma req chail
	if (per_req_num > 31)
		ax_writel(BIT(per_req_num - 32), AX_DMA_REQ_SEL_H_CLR);
	else
		ax_writel(BIT(per_req_num), AX_DMA_REQ_SEL_L_CLR);

	ax_writel(GENMASK(5, 0) << (ch % 5) * 6, AX_DMA_REQ_HS_SEL_L_SET(ch / 5));
}

static void ax_dma_per_set_lli(u32 ch, u64 src_addr, u64 dst_addr, u32 xfer_len,
			       u32 src_width, u32 dst_width, u32 burst_len,
			       u32 endian, enum dma_xfer_direction direction)
{
	u32 burst_size_byte, tr_width, blk_ts, single_xfer_len, lli_cnt, lli_idx = 0;
	ax_dma_per_lli_info_t *first_lli = NULL, *cur_lli = NULL, *prev_lli = NULL;
	char *lli_base = NULL;

	if (direction == DMA_DEV_TO_MEM) {
		tr_width = src_width;
		invalidate_dcache_range(ALIGN_DOWN(dst_addr, 64), ALIGN((dst_addr + xfer_len), 64));
	} else {
		tr_width = dst_width;
		flush_dcache_range(ALIGN_DOWN(src_addr, 64), ALIGN(src_addr + xfer_len, 64));
	}

	lli_cnt = ((xfer_len >> tr_width) + MAX_BLK_SIZE - 1) / MAX_BLK_SIZE;
	/* lli base addr must be 64 bytes aligned */
	lli_base = (char *)malloc_cache_aligned(LLI_SIZE * lli_cnt); //CONFIG_SYS_CACHELINE_SIZE = 64
	lli_base_ptr[ch] = lli_base;

	while(xfer_len) {
		single_xfer_len = xfer_len;
		blk_ts = single_xfer_len >> tr_width;
		if (blk_ts > MAX_BLK_SIZE) {
			blk_ts = MAX_BLK_SIZE;
			single_xfer_len = MAX_BLK_SIZE << tr_width;
		}

		cur_lli = (ax_dma_per_lli_info_t*)((unsigned long)lli_base + lli_idx * LLI_SIZE);
		cur_lli->ctrl.ioc = 0;
		cur_lli->ctrl.endian = endian;
		cur_lli->ctrl.dst_osd = 0xf;
		cur_lli->ctrl.src_osd = 0xf;
		cur_lli->sar = src_addr;
		cur_lli->dar = dst_addr;
		cur_lli->ctrl.last = 0;
		cur_lli->ctrl.dst_tr_wridth = tr_width;
		cur_lli->ctrl.src_tr_wridth = tr_width;

		if (direction == DMA_DEV_TO_MEM) {
			dst_addr += single_xfer_len;
			cur_lli->ctrl.src_per = 1;
			cur_lli->ctrl.sinc = 0;
			cur_lli->ctrl.dinc = 1;
			cur_lli->ctrl.arlen = DMAC_TRANS_WIDTH_16;
			cur_lli->ctrl.awlen = DMAC_TRANS_WIDTH_32;
			burst_size_byte = burst_len << cur_lli->ctrl.src_tr_wridth;
			cur_lli->ctrl.dst_tr_wridth = __ffs(single_xfer_len | 0x8 | burst_size_byte);
			cur_lli->ctrl.src_msize = burst_len;
			cur_lli->ctrl.dst_msize = burst_size_byte >> cur_lli->ctrl.dst_tr_wridth;
		} else {
			src_addr += single_xfer_len;
			cur_lli->ctrl.src_per = 0;
			cur_lli->ctrl.sinc = 1;
			cur_lli->ctrl.dinc = 0;
			cur_lli->ctrl.arlen = DMAC_TRANS_WIDTH_32;
			cur_lli->ctrl.awlen = DMAC_TRANS_WIDTH_16;
			burst_size_byte = burst_len << cur_lli->ctrl.dst_tr_wridth;
			cur_lli->ctrl.src_tr_wridth = __ffs(single_xfer_len | 0x8 | burst_size_byte);
			cur_lli->ctrl.dst_msize = burst_len;
			cur_lli->ctrl.src_msize = burst_size_byte >> cur_lli->ctrl.src_tr_wridth;
			blk_ts = single_xfer_len >> cur_lli->ctrl.src_tr_wridth;
		}
		cur_lli->data.block_ts = blk_ts;
		if (!first_lli) {
			first_lli = cur_lli;
		} else {
			prev_lli->data.llp = (u64)(long)cur_lli;
		}
		prev_lli = cur_lli;
		lli_idx++;
		xfer_len -= single_xfer_len;
	}
	cur_lli->ctrl.last = 1;
	cur_lli->data.llp = 0;

	/* handle memory cache */
	flush_dcache_range((u64)(long)lli_base,  ALIGN((u64)(long)lli_base + sizeof(ax_dma_per_lli_info_t) * lli_cnt, 64));

	ax_writel(((u64)(long)first_lli) >> 32, AX_DMAPER_CHN_LLI_H(ch));
	ax_writel(((u64)(long)first_lli) & 0xFFFFFFFF, AX_DMAPER_CHN_LLI_L(ch));
}

void axi_dma_hw_init(void)
{
	int i;

	ax_dma_per_clk_en(1);
	ax_dma_per_preset();
	ax_dma_per_areset();
	//disabled lli prefetch
	ax_dma_per_lli_prefetch(0);
	for (i = 0; i < 4; i++)
		ax_writel(0xFFFFFFFF, AX_DMA_REQ_HS_SEL_L_SET(i));
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

int axi_dma_wait_xfer_done(u32 ch)
{
	/* wait until DMA transfer done */
	while (!ax_readl(AX_DMAPER_INT_STA(ch))) ;

	/* clear int */
	ax_writel(CHN_TRANSF_DONE, AX_DMAPER_INT_CLR(ch));

	ax_dma_per_clr_req_id(ch, req_id[ch]);

	ax_dma_per_chn_en(ch, 0);
	free(lli_base_ptr[ch]);
	lli_base_ptr[ch] = 0;

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
