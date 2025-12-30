#include "axdma.h"

#define ax_writel(x, a) writel(x, ((unsigned int *)(unsigned long)a))
#define ax_readl(a) readl(((unsigned int *)(unsigned long)a))

void ax_dma_clk_en(u8 en)
{
	if (en) {
		ax_writel(DMA_CLK_EB, DMA_CLK_EB_SET);
		ax_writel(DMA_SW_RESET, DMA_SW_RESET_CLR);
	} else {
		ax_writel(DMA_SW_RESET, DMA_SW_RESET_SET);
		ax_writel(DMA_CLK_EB, DMA_CLK_EB_CLR);
	}
}

static int __ax_dma_checksum(u32 *out, u64 sar, u32 size)
{
	u32 ret = 0;
	u32 timeout_ms = 200;
	ulong start = get_timer(0);
	unsigned long long lli_buf[5] = { sar, 0, size >> 3, 0x101101AAA4436F, 0 };

	/* handle memory cache */
	flush_dcache_range((unsigned long)lli_buf, sizeof(lli_buf));
	flush_dcache_range((unsigned long)sar, size);

	*out = 0;
	ax_writel(DMA_CH0, DMA_REG_CH);
	ax_writel(DMA_OFF_TYPE_64B, DMA_REG_CTRL);
	ax_writel(((u64)(unsigned long)lli_buf >> 32 & 0xFFFFFFFF), DMA_REG_LLI_BASE_H);
	ax_writel(((u64)(unsigned long)lli_buf & 0xFFFFFFFF), DMA_REG_LLI_BASE_L);
	ax_writel(DMA_INT_MASK, DMA_REG_INT_MASK);
	ax_writel(DMA_START, DMA_REG_START);
	ax_writel(DMA_TRIG_CH0, DMA_REG_TRIG);
	while (1) {
		ret = ax_readl(DMA_REG_INT_RAW);
		if (ret & DMA_XFER_ERR) {
			return -1;
		} else if (ret & DMA_XFER_DONE) {
			ax_writel(DMA_INT_CLR, DMA_REG_INT_CLR);
			*out = ax_readl(DMA_REG_CHECKSUME);
			return 0;
		}
		if (get_timer(start) > timeout_ms) {
			return -1;
		}
	}
	return 0;
}

int ax_dma_word_checksum(u32 *out, u64 sar, int size)
{
	u32 ret;
	int tmp_size;
	u64 tmp_sar = sar;
	size &= ~0x3;
	tmp_size = size;

	if ((sar & 0x7) || (size < MIN_DATA_SIZE)) {
		return -1;
	}
#if (TEST_PLATFORM == HAPS)
	ax_dma_clk_en(1);
#endif

	*out = 0;
	while (tmp_size > 0) {
		if (__ax_dma_checksum
		    (&ret, tmp_sar, tmp_size > MAX_NODE_SIZE ? MAX_NODE_SIZE : (tmp_size & ~0x7))) {
			return -1;
		}
		(*out) += ret;
		tmp_size -= MAX_NODE_SIZE;
		tmp_sar += MAX_NODE_SIZE;
	}
	if (size & 0x7) {
		(*out) += (*((u32 *)(unsigned long)(sar + (size & ~0x7))));
	}
	return 0;
}

static void ax_dma_set_base_lli(ax_dma_lli_reg_t * lli)
{
	ax_writel(2, DMA_REG_CTRL);
	ax_writel((u32) ((u64)(unsigned long)lli >> 32), DMA_REG_LLI_BASE_H);
	ax_writel((u32) ((u64)(unsigned long)lli), DMA_REG_LLI_BASE_L);
}

static void ax_dma_enable_irq(u8 hwch, u8 en)
{
	u64 tmp;
	tmp = ax_readl(DMA_REG_INT_GLB_MASK);
	if (en) {
		tmp |= BIT(hwch);
		ax_writel(tmp, DMA_REG_INT_GLB_MASK);
		ax_writel(0x3F, DMA_REG_INT_MASK_X(hwch));
	} else {
		tmp &= ~BIT(hwch);
		ax_writel(tmp, DMA_REG_INT_GLB_MASK);
		ax_writel(0x0, DMA_REG_INT_MASK_X(hwch));
	}
}

static void ax_dma_start(u8 hwch, bool en)
{
	u64 tmp;

	tmp = ax_readl(DMA_REG_INT_GLB_MASK);
	if (en) {
		tmp |= BIT(hwch);
		ax_writel(tmp, DMA_REG_INT_GLB_MASK);
		ax_writel(0x3F, DMA_REG_INT_MASK_X(hwch));
	} else {
		tmp &= ~BIT(hwch);
		ax_writel(tmp, DMA_REG_INT_GLB_MASK);
		ax_writel(0x0, DMA_REG_INT_MASK_X(hwch));
	}
	ax_writel(en, DMA_REG_START);
	ax_writel(hwch, DMA_REG_TRIG);
}

static void ax_dma_clear_int_sta(u8 hwch)
{
	ax_writel(0x3F, DMA_REG_INT_CLR_X(hwch));
}

static int ax_dma_is_xfer_down(u8 hwch)
{
	return (ax_readl(DMA_REG_INT_RAW_X(hwch)) & 0x1);
}
#if 0
static u32 ax_dma_get_checksum(void)
{
	return (ax_readl(DMA_REG_CHECKSUME));
}
#endif
static void ax_dma_config_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli, u64 sar,
			  u64 dar, u64 block_ts, u64 tr_width, u64 wb, u64 chksum, u64 type,
			  dim_data_t * dim_buf, u64 endian, u64 ioc)
{
	if (lli == NULL) {
		return;
	}

	lli->data.block_ts = block_ts;

	lli->ctrl.sinc = 1;
	lli->ctrl.dinc = 1;
	lli->ctrl.src_tr_wridth = tr_width;
	lli->ctrl.dst_tr_wridth = tr_width;
	lli->ctrl.arlen = 2;
	lli->ctrl.awlen = 2;
	lli->ctrl.wb = wb;
	lli->ctrl.checksum = chksum;
	lli->ctrl.type = type;
	lli->ctrl.ioc = ioc;
	lli->ctrl.endian = endian;

	lli->sar = sar;
	lli->dar = dar;
	if (next_lli == NULL) {
		lli->llp = 0;
		lli->ctrl.last = 1;
	} else {
		lli->llp = (u64)(long)next_lli;
		lli->ctrl.last = 0;
	}
	if (dim_buf) {
		lli->dst_stride1 = dim_buf[0].dst_stride;
		lli->src_stride1 = dim_buf[0].src_stride;
		lli->dst_stride2 = dim_buf[1].dst_stride;
		lli->src_stride2 = dim_buf[1].src_stride;
		lli->dst_stride3 = dim_buf[2].dst_stride;
		lli->src_stride3 = dim_buf[2].dst_stride;
		lli->dst_ntile2 = dim_buf[1].dst_line;
		lli->src_ntile2 = dim_buf[1].src_line;
		lli->dst_ntile1 = dim_buf[0].dst_line;
		lli->src_ntile1 = dim_buf[0].src_line;
		lli->dst_ntile3 = dim_buf[2].dst_line;
		lli->src_ntile3 = dim_buf[2].src_line;
		lli->dst_imgw = dim_buf[0].dst_imgw;
		lli->src_imgw = dim_buf[0].src_imgw;
	}
	flush_dcache_range((u64)(long)lli, sizeof(ax_dma_lli_reg_t));
	return;
}

static void ax_dma_config_memory_init_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli,
				      u64 sar, u64 dar, u64 block_ts, u64 tr_width)
{
	dim_data_t *dim_buf = NULL;
	ax_dma_config_lli(lli, next_lli, sar, dar, block_ts, tr_width, 1, 0, AX_DMA_MEMORY_INIT,
			     dim_buf, 0, 0);
}
#if 0
static void ax_dma_config_checksum_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli,
				   u64 sar, u64 dar, u64 block_ts)
{
	dim_data_t *dim_buf = NULL;
	ax_dma_config_lli(lli, next_lli, sar, dar, block_ts, DMA_TR_8B, 0, 1, AX_DMA_1D,
			     dim_buf, 0, 0);
}

static void ax_dma_config_xd_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli, u64 sar,
			     u64 dar, u64 tr_width, u64 type, dim_data_t * dim_buf, u64 endian)
{
	ax_dma_config_lli(lli, next_lli, sar, dar, 1, tr_width, 1, 0, type, dim_buf, endian, 0);
}
#endif
static void ax_dma_config_1d_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli, u64 sar,
			     u64 dar, u64 block_ts, u64 tr_width, u64 endian)
{
	dim_data_t *dim_buf = NULL;
	ax_dma_config_lli(lli, next_lli, sar, dar, block_ts, tr_width, 1, 0, AX_DMA_1D, dim_buf,
			     endian, 0);
}

int ax_dma_xfer(u64 dst, u64 src, int size, int type)
{
	int hwch = 0;
	int tr_width = 0;
	int block_ts = 0;
	int block_ts_max = 0;
	int block_ts_tmp = 0;
	ax_dma_lli_reg_t lli;

	ax_dma_enable_irq(hwch, 1);

	if (size < 1)
		return -1;
	if (type == AX_DMA_1D)
		flush_dcache_range((u64)src, size);
	invalidate_dcache_range((u64)dst, size);
	tr_width = ffs(size | 0x8) ? ffs(size | 0x8) - 1 : 0;
	block_ts = size >> tr_width;
	do {
		//lli max size 8M
		if (tr_width > 2) {
			block_ts_max = DMA_TR_8B_MAXBLOCK;
		} else if (tr_width > 1) {
			block_ts_max = DMA_TR_4B_MAXBLOCK;
		} else {
			block_ts_max = DMA_TR_1B2B_MAXBLOCK;
		}
		if (block_ts > block_ts_max)
			block_ts_tmp = block_ts_max;
		else
			block_ts_tmp = block_ts;

		ax_dma_set_base_lli(&lli);
		if (type == AX_DMA_1D) {
			ax_dma_config_1d_lli(&lli, 0, src, dst, block_ts_tmp, tr_width, 0);
		} else if (type == AX_DMA_MEMORY_INIT) {
			if (size & 0x7)
				return -1;
			ax_dma_config_memory_init_lli(&lli, 0, src, dst, block_ts_tmp, tr_width);
		} else {
			ax_dma_clear_int_sta(hwch);
			ax_dma_enable_irq(hwch, 0);
			return -1;
		}
		ax_dma_start(hwch, 1);
		while (!ax_dma_is_xfer_down(hwch)) ;
		ax_dma_clear_int_sta(hwch);
		block_ts -= block_ts_tmp;
	} while (block_ts);

	ax_dma_enable_irq(hwch, 0);
	return 0;
}
