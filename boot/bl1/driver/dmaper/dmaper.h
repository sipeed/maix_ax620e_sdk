#ifndef __DMAPER_H__
#define __DMAPER_H__
#include "cmn.h"
#include "chip_reg.h"
#include "dma.h"

#define AX_DMAPER_BASE                  (0x48A0000)

#define AX_DMAPER_CHN_EN                (AX_DMAPER_BASE + 0x0)
#define CHN_MASK                        (0xFFFF)
#define AX_DMAPER_CTRL                  (AX_DMAPER_BASE + 0x4)
#define LLI_RD_PREFETCH_SHIFT           (17)
#define LLI_RD_PREFETCH_MASK            (1)
#define LLI_SUSPEND_EN_SHIFT            (16)
#define LLI_SUSPEND_EN_MASK             (1)
#define LLI_CHN_SUSPEND_EN_SHIFT        (0)
#define LLI_CHN_SUSPEND_EN_MASK         (0xFFFF)
#define AX_DMAPER_START                 (AX_DMAPER_BASE + 0xC)
#define AX_DMAPER_STA                   (AX_DMAPER_BASE + 0x10)
#define AX_DMAPER_TIMEOUT_REQ           (AX_DMAPER_BASE + 0x14)
#define AX_DMAPER_TIMEOUT_RD            (AX_DMAPER_BASE + 0x18)
#define REQ_TIMEMOUT_CHN_SEL_SHIFT      (5)
#define REQ_TIMEMOUT_CHN_SEL_MASK       (0xF)
#define TIMEMOUT_BYPASS_SHIFT           (4)
#define TIMEMOUT_BYPASS_MASK            (1)
#define TIMEMOUT_LVL_SEL_SHIFT          (0)
#define TIMEMOUT_LVL_MASK               (0xF)
#define AX_DMAPER_CHN_PRIORITY          (AX_DMAPER_BASE + 0x1C)
#define AX_DMAPER_CHN_LLI_H(x)          (AX_DMAPER_BASE + 0x20 + (unsigned)x * 0x8)
#define AX_DMAPER_CHN_LLI_L(x)          (AX_DMAPER_BASE + 0x24 + (unsigned)x * 0x8)
#define AX_DMAPER_CLEAR                 (AX_DMAPER_BASE + 0xA0)
#define TOTAL_CLR_EN_SHIFT              (16)
#define TOTAL_CLR_EN_MASK               (1)
#define CHN_CLR_EN_SHIFT                (0)
#define CHN_CLR_EN_MASK                 (0xFFFF)

#define AX_DMAPER_HIGH_PERF             (AX_DMAPER_BASE + 0xA4)
#define AX_DMAPER_INT_GLB_MASK          (AX_DMAPER_BASE + 0xD8)
#define AX_DMAPER_INT_GLB_RAW           (AX_DMAPER_BASE + 0xDC)
#define AX_DMAPER_INT_GLB_STA           (AX_DMAPER_BASE + 0xE0)
#define AX_DMAPER_INT_RESP_ERR_MASK     (AX_DMAPER_BASE + 0xE4)
#define AX_DMAPER_INT_RESP_ERR_CLR      (AX_DMAPER_BASE + 0xE8)
#define AX_DMAPER_INT_RESP_ERR_RAW      (AX_DMAPER_BASE + 0xEC)
#define AX_DMAPER_INT_RESP_ERR_STA      (AX_DMAPER_BASE + 0xF0)
#define AX_DMAPER_INT_MASK(x)           (AX_DMAPER_BASE + 0xF4 + (unsigned)x * 0x10)
#define AX_DMAPER_INT_CLR(x)            (AX_DMAPER_BASE + 0xF8 + (unsigned)x * 0x10)
#define AX_DMAPER_INT_RAW(x)            (AX_DMAPER_BASE + 0xFC + (unsigned)x * 0x10)
#define AX_DMAPER_INT_STA(x)            (AX_DMAPER_BASE + 0x100 + (unsigned)x * 0x10)
#define AX_DMAPER_SRC_ADDR_H(x)         (AX_DMAPER_BASE + 0x1F4 + (unsigned)x * 0x10)
#define AX_DMAPER_SRC_ADDR_L(x)         (AX_DMAPER_BASE + 0x1F8 + (unsigned)x * 0x10)
#define AX_DMAPER_DST_ADDR_H(x)         (AX_DMAPER_BASE + 0x1FC + (unsigned)x * 0x10)
#define AX_DMAPER_DST_ADDR_L(x)         (AX_DMAPER_BASE + 0x200 + (unsigned)x * 0x10)

#define AX_DMA_REQ_SEL_H_SET            PERI_SYS_GLB_DMA_SEL0_SET
#define AX_DMA_REQ_SEL_H_CLR            PERI_SYS_GLB_DMA_SEL0_CLR
#define AX_DMA_REQ_SEL_L_SET            PERI_SYS_GLB_DMA_SEL1_SET
#define AX_DMA_REQ_SEL_L_CLR            PERI_SYS_GLB_DMA_SEL1_CLR

#define AX_DMA_REQ_HS_SEL_L_SET(x)      (PERI_SYS_GLB + 0x158 - (unsigned)x * 0x8)
#define AX_DMA_REQ_HS_SEL_L_CLR(x)      (PERI_SYS_GLB + 0x15c - (unsigned)x * 0x8)

#define AX_DMAPER_ACLK_EB_SET           PERI_SYS_GLB_CLK_EB1_SET
#define AX_DMAPER_ACLK_EB_CLR           PERI_SYS_GLB_CLK_EB1_CLR
#define AX_DMAPER_ACLK_SHIFT            (0)

#define AX_DMAPER_PCLK_EB_SET           PERI_SYS_GLB_CLK_EB2_SET
#define AX_DMAPER_PCLK_EB_CLR           PERI_SYS_GLB_CLK_EB2_CLR
#define AX_DMAPER_PCLK_SHIFT            (11)

#define AX_DMAPER_RST_SET               PERI_SYS_GLB_CLK_RST0_SET
#define AX_DMAPER_RST_CLR               PERI_SYS_GLB_CLK_RST0_CLR
#define AX_DMAPER_PRST_SHIFT            (2)
#define AX_DMAPER_ARST_SHIFT            (1)

#define MAX_DMAPER_CHN                  (16)

enum {
	CHN_SUSPEND      = GENMASK(15, 0),
	AXI_LLI_RD_BUSY  = BIT(16),
	AXI_SRC_RD_BUSY  = BIT(17),
	AXI_WR_BUSY      = BIT(18),
	STA_SUSPEND      = BIT(19),
};
enum {
	INT_RESP_ERR      = BIT(16),
	RESP_ALL_ERR      = GENMASK(2, 0),
	DST_WR_RESP_ERR   = BIT(2),
	SRC_RD_RESP_ERR   = BIT(1),
	LLI_RD_RESP_ERR   = BIT(0),
	GLB_CHN_INT       = GENMASK(15, 0),
	CHN_BLOCK_DONE    = BIT(1),
	CHN_TRANSF_DONE   = BIT(0),
};

typedef struct {
	u64 vld:1;
	u64 last:1;
	u64 ioc:1;
	u64 endian:2;
	u64 src_per:1;
	u64 awlen:2;
	u64 arlen:2;
	u64 awport:3;
	u64 arport:3;
	u64 awcache:4;
	u64 arcache:4;
	u64 dst_tr_wridth:3;
	u64 src_tr_wridth:3;
	u64 dinc:1;
	u64 sinc:1;
	u64 dst_osd:4;
	u64 src_osd:4;
	u64 hs_sel_dst:1;
	u64 hs_sel_src:1;
	u64 dst_msize:10;
	u64 src_msize:10;
	u64 :2;
} ax_dma_per_lli_ctrl_t;

typedef struct {
	u64 llp:37;
	u64 block_ts:22;
	u64 :5;
} ax_dma_per_lli_data_t;

typedef struct {
	u64 sar;
	u64 dar;
	ax_dma_per_lli_ctrl_t ctrl;
	ax_dma_per_lli_data_t data;
} ax_dma_per_lli_info_t;

#endif
