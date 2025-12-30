#ifndef _AXDMA_H_
#define _AXDMA_H_
#include "cmn.h"
#include "chip_reg.h"
#include "axdma_api.h"

#define DMADIM_REG_BASE              0x10460000
#define DMADIM_REG_CH               (DMADIM_REG_BASE + 0x0)
#define DMADIM_REG_CTRL             (DMADIM_REG_BASE + 0x4)
#define DMADIM_REG_TRIG             (DMADIM_REG_BASE + 0x8)
#define DMADIM_REG_START            (DMADIM_REG_BASE + 0xC)
#define DMADIM_REG_STA              (DMADIM_REG_BASE + 0x10)
#define DMADIM_REG_LLI_BASE_H       (DMADIM_REG_BASE + 0x14)
#define DMADIM_REG_LLI_BASE_L       (DMADIM_REG_BASE + 0x18)
#define DMADIM_REG_CLEAR            (DMADIM_REG_BASE + 0x1C)
#define DMADIM_REG_INT_GLB_MASK     (DMADIM_REG_BASE + 0xEC)
#define DMADIM_REG_INT_GLB_RAW      (DMADIM_REG_BASE + 0xF0)
#define DMADIM_REG_INT_GLB_STA      (DMADIM_REG_BASE + 0xF4)
#define DMADIM_REG_INT_MASK         (DMADIM_REG_BASE + 0xF8)
#define DMADIM_REG_INT_CLR          (DMADIM_REG_BASE + 0xFC)
#define DMADIM_REG_INT_RAW          (DMADIM_REG_BASE + 0x100)
#define DMADIM_REG_INT_MASK_X(x)    (DMADIM_REG_BASE + 0xF8 + (u32)x * 0x10)
#define DMADIM_REG_INT_CLR_X(x)     (DMADIM_REG_BASE + 0xFC + (u32)x * 0x10)
#define DMADIM_REG_INT_RAW_X(x)     (DMADIM_REG_BASE + 0x100 + (u32)x * 0x10)
#define DMADIM_REG_INT_STA_X(x)     (DMADIM_REG_BASE + 0x104 + (u32)x * 0x10)
#define DMADIM_REG_CHECKSUME        (DMADIM_REG_BASE + 0x20)

#define DMADIM_CH0                  (0x1)
#define DMADIM_START                (0x1)
#define DMADIM_TRIG_CH0             (0x0)
#define DMADIM_OFF_TYPE_64B         (0x0)
#define DMADIM_INT_MASK             (0x3F)
#define DMADIM_INT_CLR              (0x3F)
#define DMADIM_XFER_ERR             (0x38)
#define DMADIM_XFER_DONE            (0x1)

#define DMADIM_PCLK_SHIFT           (14)
#define DMADIM_ACLK_SHIFT           (1)
#define DMADIM_PRST_SHIFT           (3)
#define DMADIM_ARST_SHIFT           (2)

#define DMADIM_CLK_EB               (BIT(DMADIM_PCLK_SHIFT) | BIT(DMADIM_ACLK_SHIFT))
#define DMADIM_SW_RESET             (BIT(DMADIM_PRST_SHIFT) | BIT(DMADIM_ARST_SHIFT))
#define DMADIM_CLK_EB_SET           FLASH_SYS_GLB_CLK_EB1_SET
#define DMADIM_CLK_EB_CLR           FLASH_SYS_GLB_CLK_EB1_CLR
#define DMADIM_SW_RESET_SET         FLASH_SYS_GLB_SW_RST0_SET
#define DMADIM_SW_RESET_CLR         FLASH_SYS_GLB_SW_RST0_CLR

#define MAX_NODE_SIZE               (0x800000)
#define MIN_DATA_SIZE               (0x50)

#define DMA_TR_1B2B_MAXBLOCK        (0x3FFFF8)
#define DMA_TR_4B_MAXBLOCK          (0x200000)
#define DMA_TR_8B_MAXBLOCK          (0x100000)

typedef _Bool bool;

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
typedef struct ax_dmadim_lli_reg {
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
} ax_dmadim_lli_reg_t;

typedef struct dim_data {
	u32 src_stride;
	u32 src_imgw;
	u16 src_line;
	u32 dst_stride;
	u32 dst_imgw;
	u16 dst_line;
} dim_data_t;

enum dmadim_tr_size {
	DMADIM_TR_1B = 0,
	DMADIM_TR_2B,
	DMADIM_TR_4B,
	DMADIM_TR_8B
};

enum dmadim_axi_len {
	DMADIM_AXI_1B = 0,
	DMADIM_AXI_2B,
	DMADIM_AXI_4B,
};
static void ax_dmadim_set_base_lli(ax_dmadim_lli_reg_t * lli);
static void ax_dmadim_enable_irq(u8 hwch, u8 en);
static void ax_dmadim_start(u8 hwch, bool en);
static void ax_dmadim_clear_int_sta(u8 hwch);
static int ax_dmadim_is_xfer_down(u8 hwch);
static void ax_dmadim_config_lli(ax_dmadim_lli_reg_t * lli, ax_dmadim_lli_reg_t * next_lli,
			  unsigned long sar, unsigned long dar, unsigned long block_ts,
			  unsigned long tr_width, unsigned long wb, unsigned long chksum,
			  unsigned long type, dim_data_t * dim_buf, unsigned long endian,
			  unsigned long ioc);
static void ax_dmadim_config_memory_init_lli(ax_dmadim_lli_reg_t * lli,
				      ax_dmadim_lli_reg_t * next_lli, unsigned long sar,
				      unsigned long dar, unsigned long block_ts,
				      unsigned long tr_width);
#if 0
static u32 ax_dmadim_get_checksum(void);
static void ax_dmadim_config_checksum_lli(ax_dmadim_lli_reg_t * lli, ax_dmadim_lli_reg_t * next_lli,
				   unsigned long sar, unsigned long dar, unsigned long block_ts);
void ax_dmadim_config_xd_lli(ax_dmadim_lli_reg_t * lli, ax_dmadim_lli_reg_t * next_lli,
			     unsigned long sar, unsigned long dar, unsigned long tr_width,
			     unsigned long type, dim_data_t * dim_buf, unsigned long endian);
#endif
static void ax_dmadim_config_1d_lli(ax_dmadim_lli_reg_t * lli, ax_dmadim_lli_reg_t * next_lli,
				   unsigned long sar, unsigned long dar, unsigned long block_ts,
				   unsigned long tr_width, unsigned long endian);

#endif
