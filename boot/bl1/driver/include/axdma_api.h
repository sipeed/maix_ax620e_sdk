#ifndef _AXDMA_API_H_
#define _AXDMA_API_H_

enum dmadim_type {
	DMADIM_1D = 0,
	DMADIM_2D,
	DMADIM_3D,
	DMADIM_4D,
	DMADIM_MEMORY_INIT,
};

int axi_dma_word_checksum(u32 * out, unsigned long sar, int size);
int ax_dma_xfer(unsigned long dst, unsigned long src, int size, int type);

#endif
