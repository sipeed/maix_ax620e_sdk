/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SPINOR_H__
#define __SPINOR_H__
#include "cmn.h"
/**
 * Standard SPI NOR flash operations
 */
#define SPI_NOR_ADDR_4BYTE
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
			SPI_MEM_OP(SPI_MEM_OP_CMD(0x9f, 1),			\
			SPI_MEM_OP_NO_ADDR,				\
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
#define SPINOR_OP_EN4B			0xb7	/* Enter 4-byte mode */
#define SPINOR_OP_EX4B			0xe9	/* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SPINOR_OP_BRWR			0x17	/* Bank register write */

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

#define SPINOR_OP_WREAR			0xc5	/* Write Extended Address Register */
#define SPINOR_OP_READ_4B		0x13	/* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST_4B		0x0c	/* Read data bytes (high frequency) */
#define SPINOR_OP_READ_1_1_4_4B		0x6c	/* Read data bytes (Quad Output SPI) */
#define SPINOR_OP_PP_4B			0x12	/* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_1_1_4_4B		0x34	/* Quad page program */
#define SPINOR_OP_BE_4K_4B		0x21	/* Erase 4KiB block */
#define SPINOR_OP_BE_32K_4B		0x5c	/* Erase 32KiB block */
#define SPINOR_OP_BE_64K_4B		0xdc	/* Sector erase (usually 64KiB) */

/* Status Register1 bits. */
#define SR_WIP					BIT(0)	/* Write in progress */
#define SR_WEL					BIT(1)	/* Write enable latch */
#define SR_BP0					BIT(2)	/* Block protect 0 */
#define SR_BP1					BIT(3)	/* Block protect 1 */
#define SR_BP2					BIT(4)	/* Block protect 2 */
#define SR_TB					BIT(5)	/* Top/Bottom protect */
#define SR_SRWD					BIT(7)	/* SR write protect */

#define SPINOR_MAX_ID_LEN		6
#define SPINOR_MAX_CMD_SIZE		8
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

#ifdef SPI_NOR_ADDR_4BYTE
#define CONFIG_SPI_FLASH_EON
#define CONFIG_SPI_FLASH_GIGADEVICE
#define CONFIG_SPI_FLASH_MACRONIX
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_SPI_FLASH_XMC
#define CONFIG_SPI_FLASH_ZBIT

#define SNOR_MFR_ATMEL		0x1f
#define SNOR_MFR_GIGADEVICE	0xc8
#define SNOR_MFR_INTEL		0x89
#define SNOR_MFR_ST		0x20 /* ST Micro <--> Micron */
#define SNOR_MFR_MICRON		0x2c /* ST Micro <--> Micron */
#define SNOR_MFR_MACRONIX	0xc2
#define SNOR_MFR_SPANSION	0x01
#define SNOR_MFR_SST		0xbf
#define SNOR_MFR_WINBOND	0xef /* Also used by some Spansion */
#define SNOR_MFR_EON		0x1c
struct flash_info {
	char		*name;

	/*
	 * This array stores the ID bytes.
	 * The first three bytes are the JEDIC ID.
	 * JEDEC ID zero means "no ID" (mostly older chips).
	 */
	u8		id[SPINOR_MAX_ID_LEN];
	u8		id_len;

	/* The size listed here is what works with SPINOR_OP_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	unsigned int	sector_size;
	u16		n_sectors;

	u16		page_size;
	u16		addr_width;

	u32		flags;
#define SECT_4K			BIT(0)	/* SPINOR_OP_BE_4K works uniformly */
#define SPI_NOR_NO_ERASE	BIT(1)	/* No erase command needed */
#define SST_WRITE		BIT(2)	/* use SST byte programming */
#define SPI_NOR_NO_FR		BIT(3)	/* Can't do fastread */
#define SECT_4K_PMC		BIT(4)	/* SPINOR_OP_BE_4K_PMC works uniformly */
#define SPI_NOR_DUAL_READ	BIT(5)	/* Flash supports Dual Read */
#define SPI_NOR_QUAD_READ	BIT(6)	/* Flash supports Quad Read */
#define USE_FSR			BIT(7)	/* use flag status register */
#define SPI_NOR_HAS_LOCK	BIT(8)	/* Flash supports lock/unlock via SR */
#define SPI_NOR_HAS_TB		BIT(9)	/*
					 * Flash SR has Top/Bottom (TB) protect
					 * bit. Must be used with
					 * SPI_NOR_HAS_LOCK.
					 */
#define SPI_S3AN		BIT(10)	/*
					 * Xilinx Spartan 3AN In-System Flash
					 * (MFR cannot be used for probing
					 * because it has the same value as
					 * ATMEL flashes)
					 */
#define SPI_NOR_4B_OPCODES	BIT(11)	/*
					 * Use dedicated 4byte address op codes
					 * to support memory size above 128Mib.
					 */
#define NO_CHIP_ERASE		BIT(12) /* Chip does not support chip erase */
#define SPI_NOR_SKIP_SFDP	BIT(13)	/* Skip parsing of SFDP tables */
#define USE_CLSR		BIT(14)	/* use CLSR command */
#define SPI_NOR_HAS_SST26LOCK	BIT(15)	/* Flash supports lock/unlock via BPR */
#define SPI_NOR_OCTAL_READ	BIT(16) /* Flash supports Octal Read */
};

#define JEDEC_MFR(info)		((info)->id[0])
#define JEDEC_ID(info)		(((info)->id[1]) << 8 | ((info)->id[2]))
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
