#ifndef __SPINOR_H__
#define __SPINOR_H__
#include "cmn.h"
/**
 * Standard SPI NOR flash operations
 */
#define SPINOR_READ_REG_OP(opcode, len, val)			\
			SPI_MEM_OP(SPI_MEM_OP_CMD(opcode, 1),		\
			SPI_MEM_OP_NO_ADDR,							\
			SPI_MEM_OP_NO_DUMMY,						\
			SPI_MEM_OP_DATA_IN(len, val, 1));

#define SPINOR_WRITE_REG_OP(opcode, len, buf)			\
			SPI_MEM_OP(SPI_MEM_OP_CMD(opcode, 1),		\
			SPI_MEM_OP_NO_ADDR,							\
			SPI_MEM_OP_NO_DUMMY,						\
			SPI_MEM_OP_DATA_OUT(len, buf, 1))

#define SPINOR_READID_OP(buf, len)						\
			SPI_MEM_OP(SPI_MEM_OP_CMD(0x90, 1),			\
			SPI_MEM_OP_ADDR(3, 0x00, 1),				\
			SPI_MEM_OP_NO_DUMMY,						\
			SPI_MEM_OP_DATA_IN(len, buf, 1))

#define SPINOR_BLK_ERASE_OP(addr)						\
			SPI_MEM_OP(SPI_MEM_OP_CMD(0x20, 1),			\
			SPI_MEM_OP_ADDR(3, addr, 1),				\
			SPI_MEM_OP_NO_DUMMY,						\
			SPI_MEM_OP_NO_DATA)

#define SPINOR_READ_DATA_OP(qmode, addr, buf, len)		\
			SPI_MEM_OP(SPI_MEM_OP_CMD((qmode ? 0x6b : 0x03), 1),	\
			SPI_MEM_OP_ADDR(3, addr, 1),				\
			SPI_MEM_OP_DUMMY((qmode ? 1 : 0), 1),		\
			SPI_MEM_OP_DATA_IN(len, buf, (qmode ? 4 : 1)))

#define SPINOR_PROG_LOAD(qmode, addr, buf, len)			\
			SPI_MEM_OP(SPI_MEM_OP_CMD((qmode ? 0x32 : 0x02), 1),	\
			SPI_MEM_OP_ADDR(3, addr, 1),				\
			SPI_MEM_OP_NO_DUMMY,						\
			SPI_MEM_OP_DATA_OUT(len, buf, (qmode ? 4 : 1)))

/* Flash opcodes. */
#define SPINOR_OP_RSTEN			0x66	/* Reset enable */
#define SPINOR_OP_RESET			0x99	/* Reset device */
#define SPINOR_OP_WRENVSR		0x50	/* Write enable for volatile status register */
#define SPINOR_OP_WREN			0x06	/* Write enable */
#define SPINOR_OP_WRDI			0x04	/* Write disable */
#define SPINOR_OP_RDSR			0x05	/* Read status register */
#define SPINOR_OP_WRSR			0x01	/* Write status register 1 byte */

#define SPINOR_OP_READ			0x03	/* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST		0x0b	/* Read data bytes (high frequency) */
#define SPINOR_OP_READ_QUAD		0x6b	/* Read data bytes (Quad Output SPI) */
#define SPINOR_OP_PP			0x02	/* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_QUAD		0x32	/* Quad Input page program */

#define SPINOR_OP_BE_4K			0x20	/* Erase 4KiB block */
#define SPINOR_OP_BE_32K		0x52	/* Erase 32KiB block */
#define SPINOR_OP_BE_64K		0xd8	/* Erase 64KiB block */
#define SPINOR_OP_CHIP_ERASE	0xc7	/* Erase whole flash chip */
#define SPINOR_OP_RDID			0x9f	/* Read JEDEC ID */

/* Status Register1 bits. */
#define SR_WIP					BIT(0)	/* Write in progress */
#define SR_WEL					BIT(1)	/* Write enable latch */
#define SR_BP0					BIT(2)	/* Block protect 0 */
#define SR_BP1					BIT(3)	/* Block protect 1 */
#define SR_BP2					BIT(4)	/* Block protect 2 */
#define SR_TB					BIT(5)	/* Top/Bottom protect */
#define SR_SRWD					BIT(7)	/* SR write protect */

#define SPINOR_MAX_ID_LEN		4
#define SPINOR_PAGE_SIZE		256
#define SPINOR_ERASE_SIZE		4096

#define SPINOR_DEFAULT_CLK	 	(6000000)
#define SPINOR_25M_CLK 			(25000000)
#define SPINOR_50M_CLK 			(50000000)

/*
 * QE bit may stay in different status register per different vendors
 * so we set the QE status register address in img header(nand_nor_cfg),
 * and only allow to configure quad mode after parsed img header.
 */
#if 0
/* Status Register2 bits. */
#define SPINOR_OP_RDSR2			0x35	/* Read status register 2 */
#define SPINOR_OP_WRSR2			0x31	/* Write status register 2 */
#define SR2_QUAD_EN				BIT(1)
#endif

extern int spinor_qe_cfg_parsred;
extern int spinor_qe_rdsr;
extern int spinor_qe_wrsr;
extern int spinor_qe_bit;
extern u32 spinor_rx_sample_delay[3];
extern u32 ssi_rx_sample_delay;
extern u32 spinor_phy_setting[3];
extern u32 ssi_phy_setting;
extern u32 current_bus_width;
extern u32 current_clock;

int spinor_init(u32 clk, u32 buswidth);
int spinor_quad_enable(u8 enable);
int spinor_read(u32 addr, u32 len, u8 *buf);
#if 0
int spinor_erase(u32 addr, int nsec_size);
int spinor_write(u32 addr, u32 len, const u8 *buf);
#endif

#endif
