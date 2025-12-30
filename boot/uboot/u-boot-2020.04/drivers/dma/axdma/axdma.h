#ifndef _AXDMA_H_
#define _AXDMA_H_
#include <asm/io.h>
#include <malloc.h>
#include <cpu_func.h>
#include <memalign.h>

#include "asm/arch-axera/ax620e.h"
#include "asm/arch-axera/axdma.h"

#define DMA_REG_BASE              0x10460000
#define DMA_REG_CH               (DMA_REG_BASE + 0x0)
#define DMA_REG_CTRL             (DMA_REG_BASE + 0x4)
#define DMA_REG_TRIG             (DMA_REG_BASE + 0x8)
#define DMA_REG_START            (DMA_REG_BASE + 0xC)
#define DMA_REG_STA              (DMA_REG_BASE + 0x10)
#define DMA_REG_LLI_BASE_H       (DMA_REG_BASE + 0x14)
#define DMA_REG_LLI_BASE_L       (DMA_REG_BASE + 0x18)
#define DMA_REG_CLEAR            (DMA_REG_BASE + 0x1C)
#define DMA_REG_INT_GLB_MASK     (DMA_REG_BASE + 0xEC)
#define DMA_REG_INT_GLB_RAW      (DMA_REG_BASE + 0xF0)
#define DMA_REG_INT_GLB_STA      (DMA_REG_BASE + 0xF4)
#define DMA_REG_INT_MASK         (DMA_REG_BASE + 0xF8)
#define DMA_REG_INT_CLR          (DMA_REG_BASE + 0xFC)
#define DMA_REG_INT_RAW          (DMA_REG_BASE + 0x100)
#define DMA_REG_INT_MASK_X(x)    (DMA_REG_BASE + 0xF8 + (u32)x * 0x10)
#define DMA_REG_INT_CLR_X(x)     (DMA_REG_BASE + 0xFC + (u32)x * 0x10)
#define DMA_REG_INT_RAW_X(x)     (DMA_REG_BASE + 0x100 + (u32)x * 0x10)
#define DMA_REG_INT_STA_X(x)     (DMA_REG_BASE + 0x104 + (u32)x * 0x10)
#define DMA_REG_CHECKSUME        (DMA_REG_BASE + 0x20)

#define DMA_CH0                  (0x1)
#define DMA_START                (0x1)
#define DMA_TRIG_CH0             (0x0)
#define DMA_OFF_TYPE_64B         (0x0)
#define DMA_INT_MASK             (0x3F)
#define DMA_INT_CLR              (0x3F)
#define DMA_XFER_ERR             (0x38)
#define DMA_XFER_DONE            (0x1)

#define DMA_PCLK_SHIFT           (14)
#define DMA_ACLK_SHIFT           (1)
#define DMA_PRST_SHIFT           (3)
#define DMA_ARST_SHIFT           (2)

#define DMA_CLK_EB               (BIT(DMA_PCLK_SHIFT) | BIT(DMA_ACLK_SHIFT))
#define DMA_SW_RESET             (BIT(DMA_PRST_SHIFT) | BIT(DMA_ARST_SHIFT))
#define DMA_CLK_EB_SET           FLASH_SYS_GLB_CLK_EB1_SET
#define DMA_CLK_EB_CLR           FLASH_SYS_GLB_CLK_EB1_CLR
#define DMA_SW_RESET_SET         FLASH_SYS_GLB_SW_RST0_SET
#define DMA_SW_RESET_CLR         FLASH_SYS_GLB_SW_RST0_CLR

#define MAX_NODE_SIZE               (0x800000)
#define MIN_DATA_SIZE               (0x50)

#define DMA_TR_1B2B_MAXBLOCK        (0x3FFFF8)
#define DMA_TR_4B_MAXBLOCK          (0x200000)
#define DMA_TR_8B_MAXBLOCK          (0x100000)

typedef struct {
	u64 block_ts:22;
	u64 dst_msize:10;
	u64 src_msize:10;
	u64 chn_en:16;
	u64 :6;
} ax_dma_lli_data_t;

typedef struct {
	u64 sinc:1;
	u64 dinc:1;
	u64 dst_tr_wridth:3;
	u64 src_tr_wridth:3;
	u64 wosd:6;
	u64 rosd:6;
	u64 awcache:4;
	u64 arcache:4;
	u64 awport:3;
	u64 arport:3;
	u64 arlen:2;
	u64 awlen:2;
	u64 lli_per:2;
	u64 last:1;
	u64 wb:1;
	u64 endian:2;
	u64 checksum:1;
	u64 xor_num:4;
	u64 type:3;
	u64 ioc:1;
	u64 :11;
} ax_dma_lli_ctrl_t;

/* LLI == Linked List Item */
typedef struct ax_dma_lli_reg {
	u64 sar;
	u64 dar;
	ax_dma_lli_data_t data;
	ax_dma_lli_ctrl_t ctrl;
	u64 llp;
	u32 dst_stride1;
	u32 src_stride1;
	u32 dst_stride2;
	u32 src_stride2;
	u32 dst_stride3;
	u32 src_stride3;
	u16 dst_ntile2;
	u16 src_ntile2;
	u16 dst_ntile1;
	u16 src_ntile1;
	u16 dst_ntile3;
	u16 src_ntile3;
	u16 Reserved2;
	u16 Reserved1;
	u16 dst_imgw;
	u16 src_imgw;
	u16 Reserved4;
	u16 Reserved3;
} ax_dma_lli_reg_t;

typedef struct dim_data {
	u32 src_stride;
	u32 src_imgw;
	u16 src_line;
	u32 dst_stride;
	u32 dst_imgw;
	u16 dst_line;
} dim_data_t;

enum dma_tr_size {
	DMA_TR_1B = 0,
	DMA_TR_2B,
	DMA_TR_4B,
	DMA_TR_8B
};

enum dma_axi_len {
	DMA_AXI_1B = 0,
	DMA_AXI_2B,
	DMA_AXI_4B,
};
static void ax_dma_set_base_lli(ax_dma_lli_reg_t * lli);
static void ax_dma_enable_irq(u8 hwch, u8 en);
static void ax_dma_start(u8 hwch, bool en);
static void ax_dma_clear_int_sta(u8 hwch);
static int ax_dma_is_xfer_down(u8 hwch);
static void ax_dma_config_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli, u64 sar,
			  u64 dar, u64 block_ts, u64 tr_width, u64 wb, u64 chksum, u64 type,
			  dim_data_t * dim_buf, u64 endian, u64 ioc);
static void ax_dma_config_memory_init_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli,
				      u64 sar, u64 dar, u64 block_ts, u64 tr_width);
#if 0
static u32 ax_dma_get_checksum(void);
static void ax_dma_config_checksum_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli,
				   u64 sar, u64 dar, u64 block_ts);
static void ax_dma_config_xd_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli, u64 sar,
			     u64 dar, u64 tr_width, u64 type, dim_data_t * dim_buf, u64 endian);
#endif
static void ax_dma_config_1d_lli(ax_dma_lli_reg_t * lli, ax_dma_lli_reg_t * next_lli, u64 sar,
			     u64 dar, u64 block_ts, u64 tr_width, u64 endian);


#endif
