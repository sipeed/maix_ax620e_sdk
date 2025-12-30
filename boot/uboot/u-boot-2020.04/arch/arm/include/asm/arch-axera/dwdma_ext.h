#ifndef _DWDMA_EXT_H_
#define _DWDMA_EXT_H_
#include <linux/bitops.h>
#include <asm-generic/types.h>

#define DMAC_BASE				0x48B0000
#define PERI_SYS_GLB_BASE			0x4870000
#define DMA_CLK_SET				(PERI_SYS_GLB_BASE + 0xB8)
#define DMA_CLK_CLR				(PERI_SYS_GLB_BASE + 0xBC)
#define DMA_CLK_MASK				0x8
#define DMA_RST_SET				(PERI_SYS_GLB_BASE + 0xD8)
#define DMA_RST_CLR				(PERI_SYS_GLB_BASE + 0xDC)
#define DMA_RST_MASK				0x200

#define DMAC_MAX_CHANNELS			1
#define DMAC_MAX_MASTERS			2
#define DMAC_MAX_BLK_SIZE			(1 << 22) //4M
#define LLI_SIZE				0x40

/* DMA common registers */
#define DMAC_ID					(DMAC_BASE + 0x00)
#define DMAC_COMPVER				(DMAC_BASE + 0x08)
#define DMAC_CFG				(DMAC_BASE + 0x10)
#define DMAC_CHEN_L				(DMAC_BASE + 0x18)
#define DMAC_CHEN_H				(DMAC_BASE + 0x1C)
#define DMAC_INTSTATUS				(DMAC_BASE + 0x30)
#define DMAC_COMMON_INTCLEAR			(DMAC_BASE + 0x38)
#define DMAC_COMMON_INTSTATUS_ENA		(DMAC_BASE + 0x40)
#define DMAC_COMMON_INTSIGNAL_ENA		(DMAC_BASE + 0x48)
#define DMAC_COMMON_INTSTATUS			(DMAC_BASE + 0x50)
#define DMAC_RESET				(DMAC_BASE + 0x58)

/* DMA channel registers (for x = 1; x <= DMAC_MAX_CHANNELS) */
#define DMAC_CH_SAR_L(x)			(DMAC_BASE + 0x000 + (u64)x * 0x100)
#define DMAC_CH_SAR_H(x)			(DMAC_BASE + 0x004 + (u64)x * 0x100)
#define DMAC_CH_DAR_L(x)			(DMAC_BASE + 0x008 + (u64)x * 0x100)
#define DMAC_CH_DAR_H(x)			(DMAC_BASE + 0x00C + (u64)x * 0x100)
#define DMAC_CH_BLOCK_TS(x)			(DMAC_BASE + 0x010 + (u64)x * 0x100)
#define DMAC_CH_CTL_L(x)			(DMAC_BASE + 0x018 + (u64)x * 0x100)
#define DMAC_CH_CTL_H(x)			(DMAC_BASE + 0x01C + (u64)x * 0x100)
#define DMAC_CH_CFG_L(x)			(DMAC_BASE + 0x020 + (u64)x * 0x100)
#define DMAC_CH_CFG_H(x)			(DMAC_BASE + 0x024 + (u64)x * 0x100)
#define DMAC_CH_LLP_L(x)			(DMAC_BASE + 0x028 + (u64)x * 0x100)
#define DMAC_CH_LLP_H(x)			(DMAC_BASE + 0x02C + (u64)x * 0x100)
#define DMAC_CH_STATUS(x)			(DMAC_BASE + 0x030 + (u64)x * 0x100)
#define DMAC_CH_SWHSSRC(x)			(DMAC_BASE + 0x038 + (u64)x * 0x100)
#define DMAC_CH_SWHSDST(x)			(DMAC_BASE + 0x040 + (u64)x * 0x100)
#define DMAC_CH_BLK_TFR_RESUMEREQ(x)		(DMAC_BASE + 0x048 + (u64)x * 0x100)
#define DMAC_CH_AXI_ID(x)			(DMAC_BASE + 0x050 + (u64)x * 0x100)
#define DMAC_CH_AXI_QOS(x)			(DMAC_BASE + 0x058 + (u64)x * 0x100)
#define DMAC_CH_SSTAT(x)			(DMAC_BASE + 0x060 + (u64)x * 0x100)
#define DMAC_CH_DSTAT(x)			(DMAC_BASE + 0x068 + (u64)x * 0x100)
#define DMAC_CH_SSTATAR(x)			(DMAC_BASE + 0x070 + (u64)x * 0x100)
#define DMAC_CH_DSTATAR(x)			(DMAC_BASE + 0x078 + (u64)x * 0x100)
#define DMAC_CH_INTSTATUS_ENA(x)		(DMAC_BASE + 0x080 + (u64)x * 0x100)
#define DMAC_CH_INTSTATUS(x)			(DMAC_BASE + 0x088 + (u64)x * 0x100)
#define DMAC_CH_INTSIGNAL_ENA(x)		(DMAC_BASE + 0x090 + (u64)x * 0x100)
#define DMAC_CH_INTCLEAR(x)			(DMAC_BASE + 0x098 + (u64)x * 0x100)

/* register configurations */
/* DMAC_CFG */
#define DMAC_EN_POS				0
#define DMAC_EN_MASK				BIT(DMAC_EN_POS)

#define INT_EN_POS					1
#define INT_EN_MASK				BIT(INT_EN_POS)

#define DMAC_RST_POS				0
#define DMAC_RST_MASK				BIT(DMAC_RST_POS)

#define DMAC_CHAN_EN_SHIFT			0
#define DMAC_CHAN_EN_WE_SHIFT			8

#define DMAC_CHAN_SUSP_SHIFT			16
#define DMAC_CHAN_SUSP_WE_SHIFT			24

/* CH_CTL_L */
#define CH_CTL_L_LAST_WRITE_EN			BIT(30)
#define CH_CTL_L_DST_MSIZE_POS			18
#define CH_CTL_L_SRC_MSIZE_POS			14

enum {
	DWAXIDMAC_BURST_TRANS_LEN_1		= 0,
	DWAXIDMAC_BURST_TRANS_LEN_4,
	DWAXIDMAC_BURST_TRANS_LEN_8,
	DWAXIDMAC_BURST_TRANS_LEN_16,
	DWAXIDMAC_BURST_TRANS_LEN_32,
	DWAXIDMAC_BURST_TRANS_LEN_64,
	DWAXIDMAC_BURST_TRANS_LEN_128,
	DWAXIDMAC_BURST_TRANS_LEN_256,
	DWAXIDMAC_BURST_TRANS_LEN_512,
	DWAXIDMAC_BURST_TRANS_LEN_1024
};

#define CH_CTL_L_DST_WIDTH_POS			11
#define CH_CTL_L_SRC_WIDTH_POS			8

#define CH_CTL_L_DST_INC_POS			6
#define CH_CTL_L_SRC_INC_POS			4
enum {
	DWAXIDMAC_CH_CTL_L_INC			= 0,
	DWAXIDMAC_CH_CTL_L_NOINC
};

#define CH_CTL_L_DST_MAST			BIT(2)
#define CH_CTL_L_SRC_MAST			BIT(0)

/* CH_CTL_H */
#define CH_CTL_H_ARLEN_EN			BIT(6)
#define CH_CTL_H_ARLEN_POS			7
#define CH_CTL_H_AWLEN_EN			BIT(15)
#define CH_CTL_H_AWLEN_POS			16

#define CH_CTL_H_IOC_BLK_TRF			BIT(26)
#define CH_CTL_H_LLI_LAST			BIT(30)
#define CH_CTL_H_LLI_VALID			BIT(31)


/* CH_CFG_L */
#define CH_CFG_L_DST_MULTBLK_TYPE_POS		2
#define CH_CFG_L_SRC_MULTBLK_TYPE_POS		0
enum {
	DWAXIDMAC_MBLK_TYPE_CONTIGUOUS		= 0,
	DWAXIDMAC_MBLK_TYPE_RELOAD,
	DWAXIDMAC_MBLK_TYPE_SHADOW_REG,
	DWAXIDMAC_MBLK_TYPE_LL
};

/* CH_CFG_H */
#define CH_USE_CFG2
#ifdef CH_USE_CFG2
#define CH_CFG_L_DST_PER_POS			11
#define CH_CFG_L_SRC_PER_POS			4
#define CH_CFG_H_PRIORITY_POS			15
#else
#define CH_CFG_H_DST_PER_POS			12
#define CH_CFG_H_SRC_PER_POS			7
#define CH_CFG_H_PRIORITY_POS			17
#endif
#define CH_CFG_H_HS_SEL_DST_POS			4
#define CH_CFG_H_HS_SEL_SRC_POS			3
enum {
	DWAXIDMAC_HS_SEL_HW			= 0,
	DWAXIDMAC_HS_SEL_SW
};

#define CH_CFG_H_TT_FC_POS			0
enum {
	DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC		= 0,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_PER_DMAC,
	DWAXIDMAC_TT_FC_PER_TO_MEM_SRC,
	DWAXIDMAC_TT_FC_PER_TO_PER_SRC,
	DWAXIDMAC_TT_FC_MEM_TO_PER_DST,
	DWAXIDMAC_TT_FC_PER_TO_PER_DST
};

/**
 * DW AXI DMA channel interrupts
 *
 * @DWAXIDMAC_IRQ_NONE: Bitmask of no one interrupt
 * @DWAXIDMAC_IRQ_BLOCK_TRF: Block transfer complete
 * @DWAXIDMAC_IRQ_DMA_TRF: Dma transfer complete
 * @DWAXIDMAC_IRQ_SRC_TRAN: Source transaction complete
 * @DWAXIDMAC_IRQ_DST_TRAN: Destination transaction complete
 * @DWAXIDMAC_IRQ_SRC_DEC_ERR: Source decode error
 * @DWAXIDMAC_IRQ_DST_DEC_ERR: Destination decode error
 * @DWAXIDMAC_IRQ_SRC_SLV_ERR: Source slave error
 * @DWAXIDMAC_IRQ_DST_SLV_ERR: Destination slave error
 * @DWAXIDMAC_IRQ_LLI_RD_DEC_ERR: LLI read decode error
 * @DWAXIDMAC_IRQ_LLI_WR_DEC_ERR: LLI write decode error
 * @DWAXIDMAC_IRQ_LLI_RD_SLV_ERR: LLI read slave error
 * @DWAXIDMAC_IRQ_LLI_WR_SLV_ERR: LLI write slave error
 * @DWAXIDMAC_IRQ_INVALID_ERR: LLI invalid error or Shadow register error
 * @DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR: Slave Interface Multiblock type error
 * @DWAXIDMAC_IRQ_DEC_ERR: Slave Interface decode error
 * @DWAXIDMAC_IRQ_WR2RO_ERR: Slave Interface write to read only error
 * @DWAXIDMAC_IRQ_RD2RWO_ERR: Slave Interface read to write only error
 * @DWAXIDMAC_IRQ_WRONCHEN_ERR: Slave Interface write to channel error
 * @DWAXIDMAC_IRQ_SHADOWREG_ERR: Slave Interface shadow reg error
 * @DWAXIDMAC_IRQ_WRONHOLD_ERR: Slave Interface hold error
 * @DWAXIDMAC_IRQ_LOCK_CLEARED: Lock Cleared Status
 * @DWAXIDMAC_IRQ_SRC_SUSPENDED: Source Suspended Status
 * @DWAXIDMAC_IRQ_SUSPENDED: Channel Suspended Status
 * @DWAXIDMAC_IRQ_DISABLED: Channel Disabled Status
 * @DWAXIDMAC_IRQ_ABORTED: Channel Aborted Status
 * @DWAXIDMAC_IRQ_ALL_ERR: Bitmask of all error interrupts
 * @DWAXIDMAC_IRQ_ALL: Bitmask of all interrupts
 */
enum {
	DWAXIDMAC_IRQ_NONE			= 0,
	DWAXIDMAC_IRQ_BLOCK_TRF			= BIT(0),
	DWAXIDMAC_IRQ_DMA_TRF			= BIT(1),
	DWAXIDMAC_IRQ_SRC_TRAN			= BIT(3),
	DWAXIDMAC_IRQ_DST_TRAN			= BIT(4),
	DWAXIDMAC_IRQ_SRC_DEC_ERR		= BIT(5),
	DWAXIDMAC_IRQ_DST_DEC_ERR		= BIT(6),
	DWAXIDMAC_IRQ_SRC_SLV_ERR		= BIT(7),
	DWAXIDMAC_IRQ_DST_SLV_ERR		= BIT(8),
	DWAXIDMAC_IRQ_LLI_RD_DEC_ERR		= BIT(9),
	DWAXIDMAC_IRQ_LLI_WR_DEC_ERR		= BIT(10),
	DWAXIDMAC_IRQ_LLI_RD_SLV_ERR		= BIT(11),
	DWAXIDMAC_IRQ_LLI_WR_SLV_ERR		= BIT(12),
	DWAXIDMAC_IRQ_INVALID_ERR		= BIT(13),
	DWAXIDMAC_IRQ_MULTIBLKTYPE_ERR		= BIT(14),
	DWAXIDMAC_IRQ_DEC_ERR			= BIT(16),
	DWAXIDMAC_IRQ_WR2RO_ERR			= BIT(17),
	DWAXIDMAC_IRQ_RD2RWO_ERR		= BIT(18),
	DWAXIDMAC_IRQ_WRONCHEN_ERR		= BIT(19),
	DWAXIDMAC_IRQ_SHADOWREG_ERR		= BIT(20),
	DWAXIDMAC_IRQ_WRONHOLD_ERR		= BIT(21),
	DWAXIDMAC_IRQ_LOCK_CLEARED		= BIT(27),
	DWAXIDMAC_IRQ_SRC_SUSPENDED		= BIT(28),
	DWAXIDMAC_IRQ_SUSPENDED			= BIT(29),
	DWAXIDMAC_IRQ_DISABLED			= BIT(30),
	DWAXIDMAC_IRQ_ABORTED			= BIT(31),
	DWAXIDMAC_IRQ_ALL_ERR			= (GENMASK(21, 16) | GENMASK(14, 5)),
	DWAXIDMAC_IRQ_ALL			= GENMASK(31, 0)
};

enum dma_xfer_direction{
	DMA_MEM_TO_MEM = 0,
	DMA_MEM_TO_DEV,
	DMA_DEV_TO_MEM,
};

enum {
	DMAC_TRANS_WIDTH_8 = 0,
	DMAC_TRANS_WIDTH_16,
	DMAC_TRANS_WIDTH_32,
	DMAC_TRANS_WIDTH_64,
	DMAC_TRANS_WIDTH_128,
	DMAC_TRANS_WIDTH_256,
	DMAC_TRANS_WIDTH_512,
	DMAC_TRANS_WIDTH_MAX = DMAC_TRANS_WIDTH_512
};

enum {
	DMAC_BURST_TRANS_LEN_1 	= 0,
	DMAC_BURST_TRANS_LEN_4,
	DMAC_BURST_TRANS_LEN_8,
	DMAC_BURST_TRANS_LEN_16,
	DMAC_BURST_TRANS_LEN_32,
	DMAC_BURST_TRANS_LEN_64,
	DMAC_BURST_TRANS_LEN_128,
	DMAC_BURST_TRANS_LEN_256,
	DMAC_BURST_TRANS_LEN_512,
	DMAC_BURST_TRANS_LEN_1024
};

/* LLI == Linked List Item */
typedef struct axi_dma_lli {
	u64		sar;
	u64		dar;
	u32		block_ts;
	u32		reserved;
	u64		llp;
	u32		ctl_lo;
	u32		ctl_hi;
	u32		sstat;
	u32		dstat;
	u32		status_lo;
	u32		status_hi;
	u32		reserved_lo;
	u32		reserved_hi;
}axi_dma_lli_t;

/* dma channel mapping */
enum {
	DMAC_CHAN0 = 1,
};

/* peripheral dma request interface */
#define	SSI_DMA_RX_REQ		(1)
#define	SSI_DMA_TX_REQ		(0)
#define	SSI_S_DMA_RX_REQ	(3)
#define	SSI_S_DMA_TX_REQ	(2)

#define	UART0_TX_REQ		(40)
#define	UART0_RX_REQ		(41)
#define	UART1_TX_REQ		(42)
#define	UART1_RX_REQ		(43)

void axi_dma_hw_init(void);
int axi_dma_xfer_start(u32 ch, u64 src_addr, u64 dst_addr, u32 xfer_len,
		u32 src_width, u32 dst_width, u32 burst_len,
		enum dma_xfer_direction direction, u32 per_req_num);
int axi_dma_wait_xfer_done(u32 ch);

#endif
