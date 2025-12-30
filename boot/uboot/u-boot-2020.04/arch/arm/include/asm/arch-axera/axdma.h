#ifndef _AXDMA_ARCH_H_
#define _AXDMA_ARCH_H_

enum dmadim_type {
	AX_DMA_1D = 0,
	AX_DMA_MEMORY_INIT = 4,
};

void ax_dma_clk_en(u8 en);
int ax_dma_word_checksum(u32 *out, u64 sar, int size);
int ax_dma_xfer(u64 dst, u64 src, int size, int type);

#endif
