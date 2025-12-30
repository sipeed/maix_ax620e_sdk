#ifndef __SPINAND_H
#define __SPINAND_H

#include "cmn.h"
#include "errno.h"

#define SPINAND_CONTINUOUS_READ_SUPPORT

/**
 * Standard SPI NAND flash operations
 */
#define SPINAND_RESET_OP						\
	SPI_MEM_OP(SPI_MEM_OP_CMD(0xff, 1),				\
		   SPI_MEM_OP_NO_ADDR,					\
		   SPI_MEM_OP_NO_DUMMY,					\
		   SPI_MEM_OP_NO_DATA)

#define SPINAND_WR_EN_DIS_OP(enable)					\
	SPI_MEM_OP(SPI_MEM_OP_CMD((enable) ? 0x06 : 0x04, 1),		\
		   SPI_MEM_OP_NO_ADDR,					\
		   SPI_MEM_OP_NO_DUMMY,					\
		   SPI_MEM_OP_NO_DATA)

#define SPINAND_READID_OP(ndummy, buf, len)				\
	SPI_MEM_OP(SPI_MEM_OP_CMD(0x9f, 1),				\
		   SPI_MEM_OP_NO_ADDR,					\
		   SPI_MEM_OP_DUMMY(ndummy, 1),				\
		   SPI_MEM_OP_DATA_IN(len, buf, 1))

#define SPINAND_SET_FEATURE_OP(reg, valptr)				\
	SPI_MEM_OP(SPI_MEM_OP_CMD(0x1f, 1),				\
		   SPI_MEM_OP_ADDR(1, reg, 1),				\
		   SPI_MEM_OP_NO_DUMMY,					\
		   SPI_MEM_OP_DATA_OUT(1, valptr, 1))

#define SPINAND_GET_FEATURE_OP(reg, valptr)				\
	SPI_MEM_OP(SPI_MEM_OP_CMD(0x0f, 1),				\
		   SPI_MEM_OP_ADDR(1, reg, 1),				\
		   SPI_MEM_OP_NO_DUMMY,					\
		   SPI_MEM_OP_DATA_IN(1, valptr, 1))

#define SPINAND_BLK_ERASE_OP(addr)					\
	SPI_MEM_OP(SPI_MEM_OP_CMD(0xd8, 1),				\
		   SPI_MEM_OP_ADDR(3, addr, 1),				\
		   SPI_MEM_OP_NO_DUMMY,					\
		   SPI_MEM_OP_NO_DATA)

#define SPINAND_PAGE_READ_OP(addr)					\
	SPI_MEM_OP(SPI_MEM_OP_CMD(0x13, 1),				\
		   SPI_MEM_OP_ADDR(3, addr, 1),				\
		   SPI_MEM_OP_NO_DUMMY,					\
		   SPI_MEM_OP_NO_DATA)

#define SPINAND_PAGE_READ_FROM_CACHE_OP(qmode, addr, ndummy, buf, len)	\
	SPI_MEM_OP(SPI_MEM_OP_CMD(qmode ? 0x6b : 0x03, 1),		\
		   SPI_MEM_OP_ADDR(2, addr, 1),				\
		   SPI_MEM_OP_DUMMY(ndummy, 1),				\
		   SPI_MEM_OP_DATA_IN(len, buf, qmode ? 4 : 1))

#define SPINAND_PROG_EXEC_OP(addr)					\
	SPI_MEM_OP(SPI_MEM_OP_CMD(0x10, 1),				\
		   SPI_MEM_OP_ADDR(3, addr, 1),				\
		   SPI_MEM_OP_NO_DUMMY,					\
		   SPI_MEM_OP_NO_DATA)

#define SPINAND_PROG_LOAD(qmode, addr, buf, len)			\
	SPI_MEM_OP(SPI_MEM_OP_CMD(qmode ? 0x34 : 0x84, 1),		\
		   SPI_MEM_OP_ADDR(2, addr, 1),				\
		   SPI_MEM_OP_NO_DUMMY,					\
		   SPI_MEM_OP_DATA_OUT(len, buf, qmode ? 4 : 1))

/* feature register */
#define REG_BLOCK_LOCK		0xa0
#define BL_ALL_UNLOCKED		0x00

/* configuration register */
#define REG_CFG			0xb0
#define CFG_OTP_ENABLE		BIT(6)
#define CFG_ECC_ENABLE		BIT(4)
#if defined (SPINAND_CONTINUOUS_READ_SUPPORT)
#define CFG_BUF_ENABLE		BIT(3)
#endif
#define CFG_QUAD_ENABLE		BIT(0)

/* status register */
#define REG_STATUS		0xc0
#define STATUS_BUSY		BIT(0)
#define STATUS_ERASE_FAILED	BIT(2)
#define STATUS_PROG_FAILED	BIT(3)
#define STATUS_ECC_MASK		GENMASK(5, 4)
#define STATUS_ECC_NO_BITFLIPS	(0 << 4)
#define STATUS_ECC_HAS_BITFLIPS	(1 << 4)
#define STATUS_ECC_UNCOR_ERROR	(2 << 4)

#define SPINAND_MAX_ID_LEN	4
#define SPINAND_MAX_LOGIC_BLK	34

#define SPINAND_BB_SKIPPED	1
#define SPINAND_DEBUG_ENABLE	0

#define SPINAND_DEFAULT_CLK 	(6000000)
#define SPINAND_25M_CLK 	(25000000)
#define SPINAND_50M_CLK 	(50000000)

extern int spinand_blk_size_shift;
extern int spinand_page_size_shift;
extern int spinand_oob_size_shift;
extern int spinand_oob_len;
extern int spinand_planes;
extern u32 spinand_rx_sample_delay[3];
extern u32 ssi_rx_sample_delay;
extern u32 spinand_phy_setting[3];
extern u32 ssi_phy_setting;
extern u32 current_bus_width;
extern u32 current_clock;

int spinand_init(u32 clk, u32 buswidth);
int spinand_quad_enable(u8 enable);
int spinand_read(u32 addr, u32 nbytes, char *pbuf);
#if SPINAND_DEBUG_ENABLE
int spinand_erase(u32 addr, int nblk_size);
int spinand_write(u32 addr, u32 nbytes, char *pbuf);
int spinand_block_markbad(u32 addr);
#endif

#endif /* ___SPINAND_H */

