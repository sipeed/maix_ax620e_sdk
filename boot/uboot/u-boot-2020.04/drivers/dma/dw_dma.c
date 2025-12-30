#include <asm/arch-axera/dwdma_ext.h>
#include <asm/io.h>
#include <asm/arch/ax620e.h>
#include <malloc.h>
#include <cpu_func.h>
#include <memalign.h>

static char* lli_base_ptr[DMAC_MAX_CHANNELS] = {0};

static void axi_dma_reset(void)
{
	writel(DMAC_RST_MASK, (volatile u32*)DMAC_RESET);
	/* wait until DMAC reset done */
	while(readl((volatile u32*)DMAC_RESET) & DMAC_RST_MASK);
}

static void axi_dma_enable(void)
{
	u32 val;
	val = readl((volatile u32*)DMAC_CFG);
	val |= DMAC_EN_MASK;
	writel(val, (volatile u32*)DMAC_CFG);
}

static void axi_chan_enable(u32 ch)
{
	u32 val;
	val = readl((volatile u32*)DMAC_CHEN_L);
	val |= BIT(ch-1) << DMAC_CHAN_EN_SHIFT |
		BIT(ch-1) << DMAC_CHAN_EN_WE_SHIFT;
	writel(val, (volatile u32*)DMAC_CHEN_L);
}

static void axi_chan_disable(u32 ch)
{
	u32 val;
	val = readl((volatile u32*)DMAC_CHEN_L);
	val &= ~(BIT(ch-1) << DMAC_CHAN_EN_SHIFT);
	val |= BIT(ch-1) << DMAC_CHAN_EN_WE_SHIFT;
	writel(val, (volatile u32*)DMAC_CHEN_L);
}

static u32 axi_chan_is_hw_enable(u32 ch)
{
	u32 val;
	val = readl((volatile u32*)DMAC_CHEN_L);
	return val & (BIT(ch-1) << DMAC_CHAN_EN_SHIFT);
}
/*
static void axi_dma_irq_enable()
{
	u32 val;
	val = readl((volatile u32*)DMAC_CFG);
	val |= INT_EN_MASK;
	writel(val, (volatile u32*)DMAC_CFG);
}
*/
static void axi_dma_irq_disable(void)
{
	u32 val;
	val = readl((volatile u32*)DMAC_CFG);
	val &= ~INT_EN_MASK;
	writel(val, (volatile u32*)DMAC_CFG);
}
/*
static void axi_chan_irq_disable(u32 ch, u32 irq_mask)
{
	u32 val;

	if (irq_mask == DWAXIDMAC_IRQ_ALL) {
		writel((u32)DWAXIDMAC_IRQ_NONE, (volatile u32*)DMAC_CH_INTSTATUS_ENA(ch));
	} else {
		val = readl((volatile u32*)DMAC_CH_INTSTATUS_ENA(ch));
		val &= ~irq_mask;
		writel(val, (volatile u32*)DMAC_CH_INTSTATUS_ENA(ch));
	}
}
*/
static void axi_chan_irq_set(u32 ch, u32 irq_mask)
{
	writel(irq_mask, (volatile u32*)DMAC_CH_INTSTATUS_ENA(ch));
}

static void axi_chan_irq_sig_set(u32 ch, u32 irq_mask)
{
	writel(irq_mask, (volatile u32*)DMAC_CH_INTSIGNAL_ENA(ch));
}

static void axi_chan_irq_clear(u32 ch, u32 irq_mask)
{
	writel(irq_mask, (volatile u32*)DMAC_CH_INTCLEAR(ch));
}

static u32 axi_chan_irq_read(u32 ch)
{
	return readl((volatile u32*)DMAC_CH_INTSTATUS(ch));
}
#ifndef CH_USE_CFG2
static int chip_top_rf_dma_flash_dw_sel(u32 ch, u32 dma_flash_dw_sel_ch)
{
	u32 val, shift;

	if (ch > 7) {
		printf("%s: ch %d error\n", __func__, ch);
		return -1;
	}

	shift = (ch % 4) * 8;
	if (ch / 4) {
		val = readl((volatile u32*)COMM_SYS_DMA_FLASH_DW_SEL1);
		val &= ~(0x7f << shift);
		val |= ((dma_flash_dw_sel_ch & 0x7f) << shift);
		writel(val, (volatile u32*)COMM_SYS_DMA_FLASH_DW_SEL1);
	}
	else {
		val = readl((volatile u32*)COMM_SYS_DMA_FLASH_DW_SEL0);
		val &= ~(0x7f << shift);
		val |= ((dma_flash_dw_sel_ch & 0x7f) << shift);
		writel(val, (volatile u32*)COMM_SYS_DMA_FLASH_DW_SEL0);
	}

	return 0;
}
#endif
static void axi_chan_config(u32 ch, u64 src_addr, u64 dst_addr, u32 xfer_len,
	u32 src_width, u32 dst_width, u32 burst_len,
	enum dma_xfer_direction direction, u32 per_req_num)
{
	u32 blk_ts, single_xfer_len, lliCnt, lli_idx = 0;
	u32 llp_lo = 0, llp_hi = 0, /*ctl_lo = 0, ctl_hi = 0,*/ cfg_lo = 0, cfg_hi = 0;
	axi_dma_lli_t *first_lli = NULL, *cur_lli = NULL, *prev_lli = NULL;
	char *lli_base;

	//method1: use lli
	lliCnt = ((xfer_len >> src_width) + DMAC_MAX_BLK_SIZE - 1) / DMAC_MAX_BLK_SIZE;
	/* lli base addr must be 64 bytes aligned */
	lli_base = (char *)malloc_cache_aligned(LLI_SIZE * lliCnt); //CONFIG_SYS_CACHELINE_SIZE = 64
	lli_base_ptr[ch-1] = lli_base;
	//printf("lli_base = %p lliCnt = %d\n", lli_base_ptr[ch-1], lliCnt);

	while(xfer_len) {
		single_xfer_len = xfer_len;
		blk_ts = single_xfer_len >> src_width;
		if ( blk_ts > DMAC_MAX_BLK_SIZE) {
			blk_ts = DMAC_MAX_BLK_SIZE;
			single_xfer_len = DMAC_MAX_BLK_SIZE << src_width;
		}
		/* step1: prepare lli chain */
		cur_lli = (axi_dma_lli_t*)((u64)lli_base + lli_idx * LLI_SIZE);
		cur_lli->block_ts = (blk_ts - 1);
		cur_lli->ctl_lo = (burst_len << CH_CTL_L_DST_MSIZE_POS | burst_len << CH_CTL_L_SRC_MSIZE_POS |
				dst_width << CH_CTL_L_DST_WIDTH_POS | src_width << CH_CTL_L_SRC_WIDTH_POS);

		if (direction == DMA_MEM_TO_DEV) {
			cur_lli->sar = virt_to_phys((u64*)src_addr);
			cur_lli->dar = dst_addr;
			cur_lli->ctl_lo |= (DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS |
				DWAXIDMAC_CH_CTL_L_NOINC << CH_CTL_L_DST_INC_POS);
			src_addr += single_xfer_len;
		} else if (direction == DMA_DEV_TO_MEM) {
			cur_lli->sar = src_addr;
			cur_lli->dar = virt_to_phys((u64*)dst_addr);
			cur_lli->ctl_lo |= (DWAXIDMAC_CH_CTL_L_NOINC << CH_CTL_L_SRC_INC_POS |
				DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS);
			dst_addr += single_xfer_len;
		} else { //DMA_MEM_TO_MEM
			cur_lli->sar = virt_to_phys((u64*)src_addr);
			cur_lli->dar = virt_to_phys((u64*)dst_addr);
			cur_lli->ctl_lo |= (DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS |
				DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS);
			src_addr += single_xfer_len;
			dst_addr += single_xfer_len;
		}

		cur_lli->ctl_hi = CH_CTL_H_LLI_VALID;

		if (!first_lli) {
			first_lli = cur_lli;
		} else {
			//printf("cur_lli = 0x%llx\n", virt_to_phys(cur_lli));
			prev_lli->llp = (u64)virt_to_phys(cur_lli);
		}
		prev_lli = cur_lli;

		lli_idx++;
		xfer_len -= single_xfer_len;
	}
	cur_lli->ctl_hi |= CH_CTL_H_LLI_LAST;

	flush_dcache_range((u64)lli_base, ((u64)lli_base + LLI_SIZE * lliCnt));
	/* step2: write the first lli addr to CHx_LLP register */
	if (first_lli) {
		//printf("first_lli = 0x%llx\n", virt_to_phys(first_lli));
		llp_lo = (u32)((u64)virt_to_phys(first_lli));
		llp_hi = (u32)(((u64)virt_to_phys(first_lli)) >> 32);
		writel(llp_lo, (volatile u32*)DMAC_CH_LLP_L(ch));
		writel(llp_hi, (volatile u32*)DMAC_CH_LLP_H(ch));
	}

	/* step3: configure CHx_CFG register */
	cfg_lo = (DWAXIDMAC_MBLK_TYPE_LL << CH_CFG_L_DST_MULTBLK_TYPE_POS |
		DWAXIDMAC_MBLK_TYPE_LL << CH_CFG_L_SRC_MULTBLK_TYPE_POS);
	if (direction == DMA_MEM_TO_DEV) {
		cfg_hi = (DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC << CH_CFG_H_TT_FC_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_DST_POS);
#ifdef CH_USE_CFG2
		cfg_lo |= per_req_num << CH_CFG_L_DST_PER_POS;
#else
		cfg_hi |= ((ch - 1) << CH_CFG_H_DST_PER_POS);
		if (0 != chip_top_rf_dma_flash_dw_sel((ch - 1), per_req_num))
			return;
#endif
	} else if (direction == DMA_DEV_TO_MEM) {
		cfg_hi = (DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC << CH_CFG_H_TT_FC_POS |
			DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_SRC_POS);
#ifdef CH_USE_CFG2
		cfg_lo |= per_req_num << CH_CFG_L_SRC_PER_POS;
#else
		cfg_hi |= ((ch - 1) << CH_CFG_H_SRC_PER_POS);
		if (0 != chip_top_rf_dma_flash_dw_sel((ch - 1), per_req_num))
			return;
#endif
	} else { //DMA_MEM_TO_MEM
		cfg_hi = (DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC << CH_CFG_H_TT_FC_POS);
	}
	writel(cfg_lo, (volatile u32*)DMAC_CH_CFG_L(ch));
	writel(cfg_hi, (volatile u32*)DMAC_CH_CFG_H(ch));
}

/* DMA init function */
void axi_dma_hw_init(void)
{
	u32 ch;

	writel(DMA_CLK_MASK, (volatile u32*)DMA_CLK_SET);
	writel(DMA_RST_MASK, (volatile u32*)DMA_RST_SET);
	writel(DMA_RST_MASK, (volatile u32*)DMA_RST_CLR);

	axi_dma_reset();
	axi_dma_enable();
	axi_dma_irq_disable();
	for (ch = 1; ch <= DMAC_MAX_CHANNELS; ch++) {
		axi_chan_disable(ch);
		axi_chan_irq_clear(ch, DWAXIDMAC_IRQ_ALL);
		axi_chan_irq_sig_set(ch, DWAXIDMAC_IRQ_NONE);
		axi_chan_irq_set(ch, DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_ALL_ERR);
	}
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
int axi_dma_xfer_start(u32 ch, u64 src_addr, u64 dst_addr, u32 xfer_len,
		u32 src_width, u32 dst_width, u32 burst_len,
		enum dma_xfer_direction direction, u32 per_req_num)
{
	/* check if the channel is in use */
	if (axi_chan_is_hw_enable(ch)) {
		return -1;
	}

	/* handle src/dst memory cache */
	if (direction == DMA_MEM_TO_DEV) {
		flush_dcache_range(src_addr, src_addr + xfer_len);
	} else if (direction == DMA_DEV_TO_MEM) {
		invalidate_dcache_range(dst_addr, dst_addr + xfer_len);
	} else { //DMA_MEM_TO_MEM
		flush_dcache_range(src_addr, src_addr + xfer_len);
		invalidate_dcache_range(dst_addr, dst_addr + xfer_len);
	}

	/* configure the channel */
	axi_chan_config(ch, src_addr, dst_addr, xfer_len,\
		src_width, dst_width, burst_len,\
		direction, per_req_num);

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
	free(lli_base_ptr[ch-1]);
	lli_base_ptr[ch-1] = 0;
	/* transfer done normally */
	axi_chan_disable(ch);
	return 0;
}

