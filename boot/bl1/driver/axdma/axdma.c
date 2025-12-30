#include "axdma.h"
#include "timer.h"

extern int ffs(int x);

static ax_dmadim_lli_reg_t g_lli;

static void ax_dmadim_set_base_lli(ax_dmadim_lli_reg_t * lli)
{
	writel(2, DMADIM_REG_CTRL);
	writel((u32) (((u64) (unsigned long)lli) >> 32), DMADIM_REG_LLI_BASE_H);
	writel((u32) ((unsigned long)lli), DMADIM_REG_LLI_BASE_L);
}

static void ax_dmadim_enable_irq(u8 hwch, u8 en)
{
	unsigned long tmp;
	tmp = readl(DMADIM_REG_INT_GLB_MASK);
	if (en) {
		tmp |= BIT(hwch);
		writel(tmp, DMADIM_REG_INT_GLB_MASK);
		writel(0x3F, DMADIM_REG_INT_MASK_X(hwch));
	} else {
		tmp &= ~BIT(hwch);
		writel(tmp, DMADIM_REG_INT_GLB_MASK);
		writel(0x0, DMADIM_REG_INT_MASK_X(hwch));
	}
}

static void ax_dmadim_start(u8 hwch, bool en)
{
	unsigned long tmp;

	tmp = readl(DMADIM_REG_INT_GLB_MASK);
	if (en) {
		tmp |= BIT(hwch);
		writel(tmp, DMADIM_REG_INT_GLB_MASK);
		writel(0x3F, DMADIM_REG_INT_MASK_X(hwch));
	} else {
		tmp &= ~BIT(hwch);
		writel(tmp, DMADIM_REG_INT_GLB_MASK);
		writel(0x0, DMADIM_REG_INT_MASK_X(hwch));
	}
	writel(en, DMADIM_REG_START);
	writel(hwch, DMADIM_REG_TRIG);
}

static void ax_dmadim_clear_int_sta(u8 hwch)
{
	writel(0x3F, DMADIM_REG_INT_CLR_X(hwch));
}

static int ax_dmadim_is_xfer_down(u8 hwch)
{
	return (readl(DMADIM_REG_INT_RAW_X(hwch)) & 0x1);
}
#if 0
static u32 ax_dmadim_get_checksum(void)
{
	return (readl(DMADIM_REG_CHECKSUME));
}
#endif
static void ax_dmadim_config_lli(ax_dmadim_lli_reg_t * lli, ax_dmadim_lli_reg_t * next_lli,
			  unsigned long sar, unsigned long dar, unsigned long block_ts,
			  unsigned long tr_width, unsigned long wb, unsigned long chksum,
			  unsigned long type, dim_data_t * dim_buf, unsigned long endian,
			  unsigned long ioc)
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
		lli->llp = (u64) (unsigned long)next_lli;
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
	return;
}

static void ax_dmadim_config_memory_init_lli(ax_dmadim_lli_reg_t * lli,
				      ax_dmadim_lli_reg_t * next_lli, unsigned long sar,
				      unsigned long dar, unsigned long block_ts,
				      unsigned long tr_width)
{
	dim_data_t *dim_buf = NULL;
	ax_dmadim_config_lli(lli, next_lli, sar, dar, block_ts, tr_width, 1, 0, DMADIM_MEMORY_INIT,
			     dim_buf, 0, 0);
}
#if 0
void ax_dmadim_config_checksum_lli(ax_dmadim_lli_reg_t * lli, ax_dmadim_lli_reg_t * next_lli,
				   unsigned long sar, unsigned long dar, unsigned long block_ts)
{
	dim_data_t *dim_buf = NULL;
	ax_dmadim_config_lli(lli, next_lli, sar, dar, block_ts, DMADIM_TR_8B, 0, 1, DMADIM_1D,
			     dim_buf, 0, 0);
}

void ax_dmadim_config_xd_lli(ax_dmadim_lli_reg_t * lli, ax_dmadim_lli_reg_t * next_lli,
			     unsigned long sar, unsigned long dar, unsigned long tr_width,
			     unsigned long type, dim_data_t * dim_buf, unsigned long endian)
{
	ax_dmadim_config_lli(lli, next_lli, sar, dar, 1, tr_width, 1, 0, type, dim_buf, endian, 0);
}
#endif
static void ax_dmadim_config_1d_lli(ax_dmadim_lli_reg_t * lli, ax_dmadim_lli_reg_t * next_lli,
				   unsigned long sar, unsigned long dar, unsigned long block_ts,
				   unsigned long tr_width, unsigned long endian)
{
	dim_data_t *dim_buf = NULL;
	ax_dmadim_config_lli(lli, next_lli, sar, dar, block_ts, tr_width, 1, 0, DMADIM_1D, dim_buf,
			     endian, 0);
}

int ax_dma_xfer(unsigned long dst, unsigned long src, int size, int type)
{
	int hwch = 0;
	int tr_width = 0;
	int block_ts = 0;
	int block_ts_max = 0;
	int block_ts_tmp = 0;

	ax_dmadim_enable_irq(hwch, 1);

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

		ax_dmadim_set_base_lli(&g_lli);
		if (type == DMADIM_1D) {
			ax_dmadim_config_1d_lli(&g_lli, 0, src, dst, block_ts_tmp, tr_width, 0);
		} else if (type == DMADIM_MEMORY_INIT) {
			if (size & 0x7)
				return -1;
			ax_dmadim_config_memory_init_lli(&g_lli, 0, src, dst, block_ts_tmp, tr_width);
		} else {
			ax_dmadim_clear_int_sta(hwch);
			ax_dmadim_enable_irq(hwch, 0);
			return -1;
		}
		ax_dmadim_start(hwch, 1);
		while (!ax_dmadim_is_xfer_down(hwch)) ;
		ax_dmadim_clear_int_sta(hwch);
		block_ts -= block_ts_tmp;
	} while (block_ts);

	ax_dmadim_enable_irq(hwch, 0);
	return 0;
}
