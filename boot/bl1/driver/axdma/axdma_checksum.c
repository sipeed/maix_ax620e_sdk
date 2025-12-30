#include "timer.h"
#include "cmn.h"
#include "chip_reg.h"

#define DMADIM_REG_BASE              0x10460000
#define DMADIM_REG_CH               (DMADIM_REG_BASE + 0x0)
#define DMADIM_REG_CTRL             (DMADIM_REG_BASE + 0x4)
#define DMADIM_REG_TRIG             (DMADIM_REG_BASE + 0x8)
#define DMADIM_REG_START            (DMADIM_REG_BASE + 0xC)
#define DMADIM_REG_STA              (DMADIM_REG_BASE + 0x10)
#define DMADIM_REG_LLI_BASE_H       (DMADIM_REG_BASE + 0x14)
#define DMADIM_REG_LLI_BASE_L       (DMADIM_REG_BASE + 0x18)
#define DMADIM_REG_INT_MASK         (DMADIM_REG_BASE + 0xF8)
#define DMADIM_REG_INT_CLR          (DMADIM_REG_BASE + 0xFC)
#define DMADIM_REG_INT_RAW          (DMADIM_REG_BASE + 0x100)
#define DMADIM_REG_CHECKSUME        (DMADIM_REG_BASE + 0x20)

#define DMADIM_CH0                  (0x1)
#define DMADIM_START                (0x1)
#define DMADIM_TRIG_CH0             (0x0)
#define DMADIM_OFF_TYPE_64B         (0x0)
#define DMADIM_INT_MASK             (0x3F)
#define DMADIM_INT_CLR              (0x3F)
#define DMADIM_XFER_ERR             (0x38)
#define DMADIM_XFER_DONE            (0x1)

#define MAX_NODE_SIZE               (0x800000)
#define MIN_DATA_SIZE               (0x50)

static int __ax_dma_checksum(u32 * out, u64 sar, u32 size)
{
	u32 ret = 0;
	u32 timeout_ms = 200;
	u32 wait_time, start = getCurrTime(MSEC);
	unsigned long long lli_buf[5] = { sar, 0, size >> 3, 0x101101AAA4436F, 0 };

	*out = 0;
	writel(DMADIM_CH0, DMADIM_REG_CH);
	writel(DMADIM_OFF_TYPE_64B, DMADIM_REG_CTRL);
	writel(((u64) (unsigned long)lli_buf >> 32 & 0xFFFFFFFF), DMADIM_REG_LLI_BASE_H);
	writel(((u64) (unsigned long)lli_buf & 0xFFFFFFFF), DMADIM_REG_LLI_BASE_L);
	writel(DMADIM_INT_MASK, DMADIM_REG_INT_MASK);
	writel(DMADIM_START, DMADIM_REG_START);
	writel(DMADIM_TRIG_CH0, DMADIM_REG_TRIG);
	while (1) {
		ret = readl(DMADIM_REG_INT_RAW);
		if (ret & DMADIM_XFER_ERR) {
			return -1;
		} else if (ret & DMADIM_XFER_DONE) {
			writel(DMADIM_INT_CLR, DMADIM_REG_INT_CLR);
			*out = readl(DMADIM_REG_CHECKSUME);
			return 0;
		}
		wait_time = start - getCurrTime(MSEC);
		if (wait_time > timeout_ms) {
			return -1;
		}
	}
	return 0;
}

int axi_dma_word_checksum(u32 * out, u64 sar, int size)
{
	u32 ret;
	int tmp_size;
	u64 tmp_sar = sar;
	size &= ~0x3;
	tmp_size = size;

	if ((sar & 0x7) || (size < MIN_DATA_SIZE)) {
		return -1;
	}

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
		(*out) += (*((u32 *) (unsigned long)(sar + (size & ~0x7))));
	}
	return 0;
}