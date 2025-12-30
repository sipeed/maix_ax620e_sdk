#ifndef __DW_SPI_H
#define __DW_SPI_H

#define SPI_MEM_OP_CMD(__opcode, __buswidth)			\
	{							\
		.buswidth = __buswidth,				\
		.opcode = __opcode,				\
	}

#define SPI_MEM_OP_ADDR(__nbytes, __val, __buswidth)		\
	{							\
		.nbytes = __nbytes,				\
		.val = __val,					\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_ADDR	SPI_MEM_OP_ADDR(0, 0, 0)

#define SPI_MEM_OP_DUMMY(__nbytes, __buswidth)			\
	{							\
		.nbytes = __nbytes,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_DUMMY	SPI_MEM_OP_DUMMY(0, 0)

#define SPI_MEM_OP_DATA_IN(__nbytes, __buf, __buswidth)		\
	{							\
		.dir = SPI_MEM_DATA_IN,				\
		.nbytes = __nbytes,				\
		.buf.in = __buf,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_DATA_OUT(__nbytes, __buf, __buswidth)	\
	{							\
		.dir = SPI_MEM_DATA_OUT,			\
		.nbytes = __nbytes,				\
		.buf.out = __buf,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_DATA	SPI_MEM_OP_DATA_OUT(0, 0, 0)

/**
 * enum spi_mem_data_dir - describes the direction of a SPI memory data
 *			   transfer from the controller perspective
 * @SPI_MEM_NO_DATA: no data transferred
 * @SPI_MEM_DATA_IN: data coming from the SPI memory
 * @SPI_MEM_DATA_OUT: data sent the SPI memory
 */
enum spi_mem_data_dir {
	SPI_MEM_NO_DATA,
	SPI_MEM_DATA_IN,
	SPI_MEM_DATA_OUT,
};

/**
 * struct spi_mem_op - describes a SPI memory operation
 * @cmd.buswidth: number of IO lines used to transmit the command
 * @cmd.opcode: operation opcode
 * @addr.nbytes: number of address bytes to send. Can be zero if the operation
 *		 does not need to send an address
 * @addr.buswidth: number of IO lines used to transmit the address cycles
 * @addr.val: address value. This value is always sent MSB first on the bus.
 *	      Note that only @addr.nbytes are taken into account in this
 *	      address value, so users should make sure the value fits in the
 *	      assigned number of bytes.
 * @dummy.nbytes: number of dummy bytes to send after an opcode or address. Can
 *		  be zero if the operation does not require dummy bytes
 * @dummy.buswidth: number of IO lanes used to transmit the dummy bytes
 * @data.buswidth: number of IO lanes used to send/receive the data
 * @data.dir: direction of the transfer
 * @data.buf.in: input buffer
 * @data.buf.out: output buffer
 */
struct spi_mem_op {
	struct {
		u8 buswidth;
		u8 opcode;
	} cmd;

	struct {
		u8 nbytes;
		u8 buswidth;
		u32 val;
	} addr;

	struct {
		u8 nbytes;
		u8 buswidth;
	} dummy;

	struct {
		u8 buswidth;
		enum spi_mem_data_dir dir;
		unsigned int nbytes;
		/* buf.{in,out} must be DMA-able. */
		union {
			void *in;
			const void *out;
		} buf;
	} data;
};

#define SPI_MEM_OP(__cmd, __addr, __dummy, __data)		\
	{							\
		.cmd = __cmd,					\
		.addr = __addr,					\
		.dummy = __dummy,				\
		.data = __data,					\
	}

//#define SPI_HAPS_DEBUG
/* Register offsets */
//SFC pin config reg(pin emmc)
#define PIN_CTRL				0x0
#define PIN_CTRL_SET			0x4
#define SFC_CLK					0x6c
#define SFC_MOSI_IO0			0x78
#define SFC_MISO_IO1			0x90
#define SFC_WP_IO2				0x60
#define SFC_HOLD_IO3			0x54
#define SFC_CSN0				0x84
#define SFC_CSN1				0x24

//SPI slave pin config reg(pin G1)
#define SPI_D3_S				0x48
#define SPI_D2_S				0x84
#define SPI_CLK_S				0x54
#define SPI_CS_S				0x78
#define SPI_D1_S				0x60
#define SPI_D0_S				0x6c

#define SPI_S_DUPLEX_OFFS       (3)

//GPIO0 pin config reg(pin G5)
#define FLASH_EN				0x30

#define DW_SPI_BASE				0x1a00000
#define DW_SPI_SLAVE_BASE		0x1002e000


#define DW_SPI_CTRL0			0x00
#define DW_SPI_CTRL1			0x04
#define DW_SPI_SSIENR			0x08
#define DW_SPI_MWCR				0x0c
#define DW_SPI_SER				0x10
#define DW_SPI_BAUDR			0x14
#define DW_SPI_TXFLTR			0x18
#define DW_SPI_RXFLTR			0x1c
#define DW_SPI_TXFLR			0x20
#define DW_SPI_RXFLR			0x24
#define DW_SPI_SR				0x28
#define DW_SPI_IMR				0x2c
#define DW_SPI_ISR				0x30
#define DW_SPI_RISR				0x34
#define DW_SPI_TXOICR			0x38
#define DW_SPI_RXOICR			0x3c
#define DW_SPI_RXUICR			0x40
#define DW_SPI_MSTICR			0x44
#define DW_SPI_ICR				0x48
#define DW_SPI_DMACR			0x4c
#define DW_SPI_DMATDLR			0x50
#define DW_SPI_DMARDLR			0x54
#define DW_SPI_IDR				0x58
#define DW_SPI_VERSION			0x5c
#define DW_SPI_DR				0x60
#define DW_SPI_RX_SAMPLE_DELAY	0xf0
#define DW_SPI_SPI_CTRL0		0xf4


/* Bit fields in CTRLR0 */
#define SPI_DFS_OFFSET			0
#define DFS_8_BIT				8
#define DFS_32_BIT				32

#define SPI_FRF_OFFSET			6
#define SPI_FRF_SPI				0x0
#define SPI_FRF_SSP				0x1
#define SPI_FRF_MICROWIRE		0x2
#define SPI_FRF_RESV			0x3

#define SPI_MODE_OFFSET			8
#define SPI_SCPH_OFFSET			8
#define SPI_SCOL_OFFSET			9

#define SPI_TMOD_OFFSET			10
#define SPI_TMOD_MASK			(0x3 << SPI_TMOD_OFFSET)
#define	SPI_TMOD_TR				0x0		/* xmit & recv */
#define SPI_TMOD_TO				0x1		/* xmit only */
#define SPI_TMOD_RO				0x2		/* recv only */
#define SPI_TMOD_EPROMREAD		0x3		/* eeprom read mode */

#define SPI_SLVOE_OFFSET		12
#define SPI_SLVOE_MASK			(1 << SPI_SLVOE_OFFSET)
#define SPI_SLVOE_ENABLE		0
#define SPI_SLVOE_DISABLE		1

#define SPI_SRL_OFFSET			13
#define SPI_CFS_OFFSET			16

#define SPI_SPI_FRF_OFFSET		22
#define SPI_SPI_FRF_MASK		(3 << SPI_FRF_OFFSET)
#define SPI_STANDARD_MODE		0
#define SPI_DUAL_MODE			1
#define SPI_QAUD_MODE			2

/* Bit fields in SPI_CTRLR0 */
#define SPI_TRANS_TYPE_OFFSET	0
#define SPI_BOTH_IN_SPIFRF_MODE	2
#define SPI_ADDR_L_OFFSET		2
#define SPI_ADDR_L_8_BITS		2
#define SPI_ADDR_L_32_BITS		8
#define SPI_CLK_STRETCH_EN_OFFSET	30
#define SPI_CLK_STRETCH_EN		1

/* Bit fields in TXFTLR */
#define SPI_TXFTHR_OFFSET		16

/* Bit fields in SR, 7 bits */
#define SR_MASK					GENMASK(6, 0)	/* cover 7 bits */
#define SR_BUSY					BIT(0)
#define SR_TF_NOT_FULL			BIT(1)
#define SR_TF_EMPT				BIT(2)
#define SR_RF_NOT_EMPT			BIT(3)
#define SR_RF_FULL				BIT(4)
#define SR_TX_ERR				BIT(5)
#define SR_DCOL					BIT(6)

/* Bit fields in VERSION */
#define SPI_VERSION_102A		0x3130322a
#define SPI_VERSION_103A		0x3130332a

#define SPI_MAX_BUS_CLK			24000000
#define SPI_NPLL_200M_MAX_BUS_CLK	200000000
#define SPI_CPLL_312M_MAX_BUS_CLK	312000000
#define SPI_CPLL_416M_MAX_BUS_CLK	416000000

#define RX_TIMEOUT				10000
#define DW_SPI_FIFO_LEN			64
#define SPI_MST_DMA_TXRX_DLR	(DW_SPI_FIFO_LEN >> 1) //half fifo depth
#define DW_SPI_SLV_FIFO_LEN		32
#define SPI_SLV_DMA_TXRX_DLR	16

#define SPI_MST_USE_DMA
#define SPI_SLV_USE_DMA

#ifdef SPI_MST_USE_DMA
#define MAX_WRITE_SIZE			(32 << 10) //32KB
#define MAX_READ_SIZE			(32 << 10) //32KB
#else
#define MAX_WRITE_SIZE			64
#define MAX_READ_SIZE			64
#endif

#define SPI_XFER_BEGIN			BIT(0)	/* Assert CS before transfer */
#define SPI_XFER_END			BIT(1)	/* Deassert CS after transfer */
#define SPI_XFER_ONCE			(SPI_XFER_BEGIN | SPI_XFER_END)

#define NOR_CS				0 //cs0
#define NAND_CS				1 //cs1
//#define CURR_CS				27 //nand & nor both use cs0 in formal version
#define SPI_CS_GPIO(x)			((x) > 0 ? 25 : 29)
#define SPI_CS_NUM			2

#define CLK_SPI_S_SEL_OFFSET    (20)
#define CLK_SPI_S_SEL_MASK      (0x3 << CLK_SPI_S_SEL_OFFSET)
#define CLK_SPI_S_SEL_CPLL_24M  (0x0)
#define CLK_SPI_S_SEL_EPLL_125M (0x1)
#define CLK_SPI_S_SEL_CPLL_156M (0x2)
#define CLK_SPI_S_SEL_CPLL_208M (0x3)
#define CLK_SPI_S_SEL_MAX       (0x4)

struct dw_poll_priv {
	u32 entries;
	void *tx;
	void *rx;
	u32 fifowidth;
};

/* spi master APIs */
void spi_hw_init();
int spi_mem_adjust_op_size(struct spi_mem_op *op);
int spi_mem_exec_op(const struct spi_mem_op *op);

/* spi slave APIs */
void spi_slv_hw_init();
void spi_slv_quad_mode(u8 enable);
void spi_slv_freq_boost(u8 sel);
int spi_slv_read(unsigned int nbytes, void *din, u32 timeout_ms);
int spi_slv_write(unsigned int nbytes, void *dout, u32 timeout_ms);
#if defined(AX620E_NAND)
extern void external_cs_manage(u32 on);
#endif

#endif
