/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "spinor.h"
#include "dw_spi.h"
#include "ax_common.h"
#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
// #include "timer.h"

#ifdef SPI_NOR_ADDR_4BYTE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
/* Exclude chip names for SPL to save space */
#define INFO_NAME(_name) .name = _name,

/* Used when the "_ext_id" is two bytes at most */
#define INFO(_name, _jedec_id, _ext_id, _sector_size, _n_sectors, _flags)	\
		INFO_NAME(_name)					\
		.id = {							\
			((_jedec_id) >> 16) & 0xff,			\
			((_jedec_id) >> 8) & 0xff,			\
			(_jedec_id) & 0xff,				\
			((_ext_id) >> 8) & 0xff,			\
			(_ext_id) & 0xff,				\
			},						\
		.id_len = (!(_jedec_id) ? 0 : (3 + ((_ext_id) ? 2 : 0))),	\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = 256,					\
		.flags = (_flags),

#define INFO6(_name, _jedec_id, _ext_id, _sector_size, _n_sectors, _flags)	\
		INFO_NAME(_name)					\
		.id = {							\
			((_jedec_id) >> 16) & 0xff,			\
			((_jedec_id) >> 8) & 0xff,			\
			(_jedec_id) & 0xff,				\
			((_ext_id) >> 16) & 0xff,			\
			((_ext_id) >> 8) & 0xff,			\
			(_ext_id) & 0xff,				\
			},						\
		.id_len = 6,						\
		.sector_size = (_sector_size),				\
		.n_sectors = (_n_sectors),				\
		.page_size = 256,					\
		.flags = (_flags),

/* NOTE: double check command sets and memory organization when you add
 * more nor chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 *
 * All newly added entries should describe *hardware* and should use SECT_4K
 * (or SECT_4K_PMC) if hardware supports erasing 4 KiB sectors. For usage
 * scenarios excluding small sectors there is config option that can be
 * disabled: CONFIG_SPI_FLASH_USE_4K_SECTORS.
 * For historical (and compatibility) reasons (before we got above config) some
 * old entries may be missing 4K flag.
 */
const struct flash_info spi_nor_ids[] = {
#ifdef CONFIG_SPI_FLASH_ATMEL		/* ATMEL */
	/* Atmel -- some are (confusingly) marketed as "DataFlash" */
	{ INFO("at26df321",	0x1f4700, 0, 64 * 1024, 64, SECT_4K) },
	{ INFO("at25df321a",	0x1f4701, 0, 64 * 1024, 64, SECT_4K) },

	{ INFO("at45db011d",	0x1f2200, 0, 64 * 1024,   4, SECT_4K) },
	{ INFO("at45db021d",	0x1f2300, 0, 64 * 1024,   8, SECT_4K) },
	{ INFO("at45db041d",	0x1f2400, 0, 64 * 1024,   8, SECT_4K) },
	{ INFO("at45db081d",	0x1f2500, 0, 64 * 1024,  16, SECT_4K) },
	{ INFO("at45db161d",	0x1f2600, 0, 64 * 1024,  32, SECT_4K) },
	{ INFO("at45db321d",	0x1f2700, 0, 64 * 1024,  64, SECT_4K) },
	{ INFO("at45db641d",	0x1f2800, 0, 64 * 1024, 128, SECT_4K) },
	{ INFO("at25sl321",	0x1f4216, 0, 64 * 1024,  64, SECT_4K) },
	{ INFO("at26df081a", 	0x1f4501, 0, 64 * 1024,  16, SECT_4K) },
#endif
#ifdef CONFIG_SPI_FLASH_EON		/* EON */
	/* EON -- en25xxx */
	{ INFO("en25q40b",   0x1c3013, 0, 64 * 1024,   8, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("en25q32b",   0x1c3016, 0, 64 * 1024,   64, 0) },
	{ INFO("en25q64",    0x1c3017, 0, 64 * 1024,  128, SECT_4K) },
	{ INFO("en25qh128",  0x1c7018, 0, 64 * 1024,  256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_HAS_LOCK) },
	{ INFO("en25s64",    0x1c3817, 0, 64 * 1024,  128, SECT_4K) },
#endif
#ifdef CONFIG_SPI_FLASH_GIGADEVICE	/* GIGADEVICE */
	/* GigaDevice */
	{
		INFO("gd25q16", 0xc84015, 0, 64 * 1024,  32,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("gd25q32", 0xc84016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("gd25lq32", 0xc86016, 0, 64 * 1024, 64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("gd25q64", 0xc84017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("gd25q128", 0xc84018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("gd25lq128", 0xc86018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
#endif
#ifdef CONFIG_SPI_FLASH_ISSI		/* ISSI */
	/* ISSI */
	{ INFO("is25lq040b", 0x9d4013, 0, 64 * 1024,   8,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("is25lp032",  0x9d6016, 0, 64 * 1024,  64, 0) },
	{ INFO("is25lp064",  0x9d6017, 0, 64 * 1024, 128, 0) },
	{ INFO("is25lp128",  0x9d6018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ) },
	{ INFO("is25lp256",  0x9d6019, 0, 64 * 1024, 512,
			SECT_4K | SPI_NOR_DUAL_READ) },
	{ INFO("is25wp032",  0x9d7016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("is25wp064",  0x9d7017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("is25wp128",  0x9d7018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("is25wp256",  0x9d7019, 0, 64 * 1024, 512,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
#endif
#ifdef CONFIG_SPI_FLASH_MACRONIX	/* MACRONIX */
	/* Macronix */
	{ INFO("mx25l2005a",  0xc22012, 0, 64 * 1024,   4, SECT_4K) },
	{ INFO("mx25l4005a",  0xc22013, 0, 64 * 1024,   8, SECT_4K) },
	{ INFO("mx25l8005",   0xc22014, 0, 64 * 1024,  16, 0) },
	{ INFO("mx25l1606e",  0xc22015, 0, 64 * 1024,  32, SECT_4K) },
	{ INFO("mx25l3205d",  0xc22016, 0, 64 * 1024,  64, SECT_4K) },
	{ INFO("mx25l6405d",  0xc22017, 0, 64 * 1024, 128, SECT_4K) },
	{ INFO("mx25u2033e",  0xc22532, 0, 64 * 1024,   4, SECT_4K) },
	{ INFO("mx25u1635e",  0xc22535, 0, 64 * 1024,  32, SECT_4K) },
	{ INFO("mx25u3235f",  0xc22536, 0, 4 * 1024,  1024, SECT_4K) },
	{ INFO("mx25u6435f",  0xc22537, 0, 64 * 1024, 128, SECT_4K) },
	{ INFO("mx25l12805d", 0xc22018, 0, 64 * 1024, 256, 0) },
	{ INFO("mx25l12855e", 0xc22618, 0, 64 * 1024, 256, 0) },
	{ INFO("mx25l25635e", 0xc22019, 0, 64 * 1024, 512, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("mx25u25635f", 0xc22539, 0, 64 * 1024, 512, SECT_4K | SPI_NOR_4B_OPCODES) },
	{ INFO("mx25l25655e", 0xc22619, 0, 64 * 1024, 512, 0) },
	{ INFO("mx66l51235l", 0xc2201a, 0, 64 * 1024, 1024, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
	{ INFO("mx66u51235f", 0xc2253a, 0, 64 * 1024, 1024, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
	{ INFO("mx66u2g45g",  0xc2253c, 0, 64 * 1024, 4096, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
	{ INFO("mx66l1g45g",  0xc2201b, 0, 64 * 1024, 2048, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("mx25l1633e",  0xc22415, 0, 64 * 1024,   32, SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES | SECT_4K) },
#endif

#ifdef CONFIG_SPI_FLASH_STMICRO		/* STMICRO */
	/* Micron */
	{ INFO("n25q016a",	 0x20bb15, 0, 64 * 1024,   32, SECT_4K | SPI_NOR_QUAD_READ) },
	{ INFO("n25q032",	 0x20ba16, 0, 64 * 1024,   64, SPI_NOR_QUAD_READ) },
	{ INFO("n25q032a",	0x20bb16, 0, 64 * 1024,   64, SPI_NOR_QUAD_READ) },
	{ INFO("n25q064",     0x20ba17, 0, 64 * 1024,  128, SECT_4K | SPI_NOR_QUAD_READ) },
	{ INFO("n25q064a",    0x20bb17, 0, 64 * 1024,  128, SECT_4K | SPI_NOR_QUAD_READ) },
	{ INFO("n25q128a11",  0x20bb18, 0, 64 * 1024,  256, SECT_4K | SPI_NOR_QUAD_READ) },
	{ INFO("n25q128a13",  0x20ba18, 0, 64 * 1024,  256, SECT_4K | SPI_NOR_QUAD_READ) },
	{ INFO6("mt25ql256a",    0x20ba19, 0x104400, 64 * 1024,  512, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES | USE_FSR) },
	{ INFO("n25q256a",    0x20ba19, 0, 64 * 1024,  512, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_FSR) },
	{ INFO6("mt25qu256a",  0x20bb19, 0x104400, 64 * 1024,  512, SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES | USE_FSR) },
	{ INFO("n25q256ax1",  0x20bb19, 0, 64 * 1024,  512, SECT_4K | SPI_NOR_QUAD_READ | USE_FSR) },
	{ INFO6("mt25qu512a",  0x20bb20, 0x104400, 64 * 1024, 1024,
		 SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES |
		 USE_FSR) },
	{ INFO("n25q512a",    0x20bb20, 0, 64 * 1024, 1024, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ) },
	{ INFO6("mt25ql512a",  0x20ba20, 0x104400, 64 * 1024, 1024, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
	{ INFO("n25q512ax3",  0x20ba20, 0, 64 * 1024, 1024, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ) },
	{ INFO("n25q00",      0x20ba21, 0, 64 * 1024, 2048, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ | NO_CHIP_ERASE) },
	{ INFO("n25q00a",     0x20bb21, 0, 64 * 1024, 2048, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ | NO_CHIP_ERASE) },
	{ INFO("mt25qu02g",   0x20bb22, 0, 64 * 1024, 4096, SECT_4K | USE_FSR | SPI_NOR_QUAD_READ | NO_CHIP_ERASE) },
	{ INFO("mt35xu512aba", 0x2c5b1a, 0,  128 * 1024,  512, USE_FSR | SPI_NOR_4B_OPCODES) },
	{ INFO("mt35xu02g",  0x2c5b1c, 0, 128 * 1024,  2048, USE_FSR | SPI_NOR_4B_OPCODES) },
#endif
#ifdef CONFIG_SPI_FLASH_SPANSION	/* SPANSION */
	/* Spansion/Cypress -- single (large) sector size only, at least
	 * for the chips listed here (without boot sectors).
	 */
	{ INFO("s25sl032p",  0x010215, 0x4d00,  64 * 1024,  64, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("s25sl064p",  0x010216, 0x4d00,  64 * 1024, 128, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("s25fl256s0", 0x010219, 0x4d00, 256 * 1024, 128, USE_CLSR) },
	{ INFO("s25fl256s1", 0x010219, 0x4d01,  64 * 1024, 512, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_CLSR) },
	{ INFO6("s25fl512s",  0x010220, 0x4d0081, 256 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_CLSR) },
	{ INFO("s25fl512s_256k",  0x010220, 0x4d00, 256 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_CLSR) },
	{ INFO("s25fl512s_64k",  0x010220, 0x4d01, 64 * 1024, 1024, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_CLSR) },
	{ INFO("s25fl512s_512k", 0x010220, 0x4f00, 256 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_CLSR) },
	{ INFO("s25sl12800", 0x012018, 0x0300, 256 * 1024,  64, 0) },
	{ INFO("s25sl12801", 0x012018, 0x0301,  64 * 1024, 256, 0) },
	{ INFO6("s25fl128s",  0x012018, 0x4d0180, 64 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_CLSR) },
	{ INFO("s25fl129p0", 0x012018, 0x4d00, 256 * 1024,  64, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_CLSR) },
	{ INFO("s25fl129p1", 0x012018, 0x4d01,  64 * 1024, 256, SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | USE_CLSR) },
	{ INFO("s25sl008a",  0x010213,      0,  64 * 1024,  16, 0) },
	{ INFO("s25sl016a",  0x010214,      0,  64 * 1024,  32, 0) },
	{ INFO("s25sl032a",  0x010215,      0,  64 * 1024,  64, 0) },
	{ INFO("s25sl064a",  0x010216,      0,  64 * 1024, 128, 0) },
	{ INFO("s25fl116k",  0x014015,      0,  64 * 1024,  32, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("s25fl164k",  0x014017,      0,  64 * 1024, 128, SECT_4K) },
	{ INFO("s25fl208k",  0x014014,      0,  64 * 1024,  16, SECT_4K | SPI_NOR_DUAL_READ) },
	{ INFO("s25fl064l",  0x016017,      0,  64 * 1024, 128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
	{ INFO("s25fl128l",  0x016018,      0,  64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
#endif
#ifdef CONFIG_SPI_FLASH_SST		/* SST */
	/* SST -- large erase sizes are "overlays", "sectors" are 4K */
	{ INFO("sst25vf040b", 0xbf258d, 0, 64 * 1024,  8, SECT_4K | SST_WRITE) },
	{ INFO("sst25vf080b", 0xbf258e, 0, 64 * 1024, 16, SECT_4K | SST_WRITE) },
	{ INFO("sst25vf016b", 0xbf2541, 0, 64 * 1024, 32, SECT_4K | SST_WRITE) },
	{ INFO("sst25vf032b", 0xbf254a, 0, 64 * 1024, 64, SECT_4K | SST_WRITE) },
	{ INFO("sst25vf064c", 0xbf254b, 0, 64 * 1024, 128, SECT_4K) },
	{ INFO("sst25wf512",  0xbf2501, 0, 64 * 1024,  1, SECT_4K | SST_WRITE) },
	{ INFO("sst25wf010",  0xbf2502, 0, 64 * 1024,  2, SECT_4K | SST_WRITE) },
	{ INFO("sst25wf020",  0xbf2503, 0, 64 * 1024,  4, SECT_4K | SST_WRITE) },
	{ INFO("sst25wf020a", 0x621612, 0, 64 * 1024,  4, SECT_4K) },
	{ INFO("sst25wf040b", 0x621613, 0, 64 * 1024,  8, SECT_4K) },
	{ INFO("sst25wf040",  0xbf2504, 0, 64 * 1024,  8, SECT_4K | SST_WRITE) },
	{ INFO("sst25wf080",  0xbf2505, 0, 64 * 1024, 16, SECT_4K | SST_WRITE) },
	{ INFO("sst26vf064b", 0xbf2643, 0, 64 * 1024, 128, SECT_4K | SPI_NOR_HAS_SST26LOCK | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("sst26wf016",  0xbf2651, 0, 64 * 1024,  32, SECT_4K | SPI_NOR_HAS_SST26LOCK) },
	{ INFO("sst26wf032",  0xbf2622, 0, 64 * 1024,  64, SECT_4K | SPI_NOR_HAS_SST26LOCK) },
	{ INFO("sst26wf064",  0xbf2643, 0, 64 * 1024, 128, SECT_4K | SPI_NOR_HAS_SST26LOCK) },
#endif
#ifdef CONFIG_SPI_FLASH_STMICRO		/* STMICRO */
	/* ST Microelectronics -- newer production may have feature updates */
	{ INFO("m25p10",  0x202011,  0,  32 * 1024,   4, 0) },
	{ INFO("m25p20",  0x202012,  0,  64 * 1024,   4, 0) },
	{ INFO("m25p40",  0x202013,  0,  64 * 1024,   8, 0) },
	{ INFO("m25p80",  0x202014,  0,  64 * 1024,  16, 0) },
	{ INFO("m25p16",  0x202015,  0,  64 * 1024,  32, 0) },
	{ INFO("m25p32",  0x202016,  0,  64 * 1024,  64, 0) },
	{ INFO("m25p64",  0x202017,  0,  64 * 1024, 128, 0) },
	{ INFO("m25p128", 0x202018,  0, 256 * 1024,  64, 0) },
	{ INFO("m25pe16", 0x208015,  0, 64 * 1024, 32, SECT_4K) },
	{ INFO("m25px16", 0x207115,  0, 64 * 1024, 32, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("m25px64", 0x207117,  0, 64 * 1024, 128, 0) },
#endif
#ifdef CONFIG_SPI_FLASH_WINBOND		/* WINBOND */
	/* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
	{ INFO("w25p80", 0xef2014, 0x0,	64 * 1024,    16, 0) },
	{ INFO("w25p16", 0xef2015, 0x0,	64 * 1024,    32, 0) },
	{ INFO("w25p32", 0xef2016, 0x0,	64 * 1024,    64, 0) },
	{ INFO("w25x05", 0xef3010, 0, 64 * 1024,  1,  SECT_4K) },
	{ INFO("w25x40", 0xef3013, 0, 64 * 1024,  8,  SECT_4K) },
	{ INFO("w25x16", 0xef3015, 0, 64 * 1024,  32, SECT_4K) },
	{
		INFO("w25q16dw", 0xef6015, 0, 64 * 1024,  32,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{ INFO("w25x32", 0xef3016, 0, 64 * 1024,  64, SECT_4K) },
	{ INFO("w25q20cl", 0xef4012, 0, 64 * 1024,  4, SECT_4K) },
	{ INFO("w25q20bw", 0xef5012, 0, 64 * 1024,  4, SECT_4K) },
	{ INFO("w25q20ew", 0xef6012, 0, 64 * 1024,  4, SECT_4K) },
	{ INFO("w25q32", 0xef4016, 0, 64 * 1024,  64, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{
		INFO("w25q32dw", 0xef6016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("w25q32jv", 0xef7016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{ INFO("w25x64", 0xef3017, 0, 64 * 1024, 128, SECT_4K) },
	{
		INFO("w25q64dw", 0xef6017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("w25q64jv", 0xef7017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("w25q128fw", 0xef6018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("w25q128jv", 0xef7018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("w25q128jw", 0xef8018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("w25q256fw", 0xef6019, 0, 64 * 1024, 512,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{
		INFO("w25q256jw", 0xef7019, 0, 64 * 1024, 512,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB)
	},
	{ INFO("w25q80", 0xef5014, 0, 64 * 1024,  16, SECT_4K) },
	{ INFO("w25q80bl", 0xef4014, 0, 64 * 1024,  16, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("w25q16cl", 0xef4015, 0, 64 * 1024,  32, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("w25q64cv", 0xef4017, 0, 64 * 1024,  128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("w25q128", 0xef4018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("w25q256", 0xef4019, 0, 64 * 1024, 512, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
#endif
#ifdef CONFIG_SPI_FLASH_XMC
	/* XMC (Wuhan Xinxin Semiconductor Manufacturing Corp.) */
	{ INFO("XM25QH64A", 0x207017, 0, 64 * 1024, 128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("XM25QH128A", 0x207018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("XM25QH128AHIG", 0x204018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
#endif
#ifdef CONFIG_SPI_FLASH_ZBIT
	/* ZBIT (Zbit Semiconductor Manufacturing Corp.) */
	{ INFO("ZB25VQ64", 0x5e4017, 0, 64 * 1024, 128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ INFO("ZB25VQ128", 0x5e4018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
#endif
	{ },
};
static u32 spinor_erasesize = 0x10000;
static u32 spinor_size = 0x1000000;
static u8 spinor_addr_width = 3;
static u8 spinor_read_opcode = SPINOR_OP_READ;
static u8 spinor_program_opcode = SPINOR_OP_PP;
static u8 spinor_erase_opcode = SPINOR_OP_BE_64K;
static u8 spinor_cmd_buf[SPINOR_MAX_CMD_SIZE];
#endif

static int spinor_initialized[SPI_CS_NUM] = {0};
static int spinor_qmode = 0;
int spinor_qe_cfg_parsred = 0;
int spinor_qe_rdsr = 0x35;
int spinor_qe_wrsr = 0x31;
int spinor_qe_bit = 1;
u32 spinor_rx_sample_delay[3] = {0};
u32 spinor_phy_setting[3] = {0};

extern u8 curr_cs;

static int spinor_read_reg(u8 opcode, u8 *val, int len)
{
	struct spi_mem_op op = SPINOR_READ_REG_OP(opcode, len, val);
	return spi_mem_exec_op(&op);
}

static int spinor_write_reg(u8 opcode, u8 *buf, int len)
{
	struct spi_mem_op op = SPINOR_WRITE_REG_OP(opcode, len, buf);
	return spi_mem_exec_op(&op);
}

static int read_sr(u8 opcode, void *val)
{
	return spinor_read_reg(opcode, val, 1);
}

static int write_sr(u8 opcode, u8 val)
{
	return spinor_write_reg(opcode, &val, 1);
}

#define SPI_NOR_WRITE_ENABLE_FOR_VOLATILE_STATUS_REGISTER
#ifdef SPI_NOR_WRITE_ENABLE_FOR_VOLATILE_STATUS_REGISTER
static int write_enable_volatile()
{
	AX_LOG_ERROR("========== op: 0x%x ==============\n", SPINOR_OP_WRENVSR);
	return spinor_write_reg(SPINOR_OP_WRENVSR, NULL, 0);
}
#endif

static int write_enable()
{
	return spinor_write_reg(SPINOR_OP_WREN, NULL, 0);
}

static int spinor_wait_till_ready()
{
	unsigned long timeout = 100000; //MAX = 1600ms for 32K blk erase
	u8 sr = 0;
	int ret;

	while (timeout--) {
		ret = read_sr(SPINOR_OP_RDSR, &sr);
		if (ret < 0)
			return ret;

		if(!(sr & SR_WIP))
			break;
	}

	if(timeout < 0)
		return -1;

	return 0;
}

static int spinor_read_data(u32 addr, u32 len, u8 *buf)
{
	struct spi_mem_op op = SPINOR_READ_DATA_OP(spinor_qmode, addr, buf, len);
	u32 remaining = len;
	int ret;

#ifdef SPI_NOR_ADDR_4BYTE
	if (4 == spinor_addr_width) {
		op.cmd.opcode = spinor_read_opcode;
		op.addr.nbytes = spinor_addr_width;
	}
#endif
	while (remaining) {
		op.data.nbytes = remaining;
		ret = spi_mem_adjust_op_size(&op);
		if (ret)
			return ret;

		ret = spi_mem_exec_op(&op);
		if (ret)
		{
			return ret;
		}

		op.addr.val += op.data.nbytes;
		remaining -= op.data.nbytes;
		op.data.buf.in += op.data.nbytes;
	}

	return len;
}

#ifdef SPI_NOR_ADDR_4BYTE
static int write_disable()
{
	return spinor_write_reg(SPINOR_OP_WRDI, NULL, 0);
}
#endif

#if 0
static int spinor_write_data(u32 addr, u32 len, const u8 *buf)
{
	struct spi_mem_op op = SPINOR_PROG_LOAD(spinor_qmode, addr, buf, len);
	int ret;

	ret = spi_mem_adjust_op_size(&op);
	if (ret)
		return ret;
	op.data.nbytes = len < op.data.nbytes ? len : op.data.nbytes;

	ret = spi_mem_exec_op(&op);
	if (ret)
		return ret;

	return op.data.nbytes;
}

static int spinor_erase_sector(u32 addr)
{
	struct spi_mem_op op = SPINOR_BLK_ERASE_OP(addr);
	return spi_mem_exec_op(&op);
}

int spinor_erase(u32 addr, int nsec_size)
{
	int ret;

	while (nsec_size > 0) {
		write_enable();

		ret = spinor_erase_sector(addr);
		if (ret)
			goto erase_err;

		addr += SPINOR_ERASE_SIZE;
		nsec_size -= SPINOR_ERASE_SIZE;

		ret = spinor_wait_till_ready();
		if (ret)
			goto erase_err;
	}

erase_err:
	write_disable();

	return ret;
}
#endif

#ifdef SPI_NOR_ADDR_4BYTE
static const struct flash_info *spinor_read_id(void)
{
	int	tmp;
	u8	id[SPINOR_MAX_ID_LEN];
	const struct flash_info	*info;
	struct spi_mem_op op = SPINOR_READID_OP(id, SPINOR_MAX_ID_LEN);

	tmp = spi_mem_exec_op(&op);
	if (tmp < 0) {
		AX_LOG_ERROR("error %d reading JEDEC ID\n", tmp);
		return NULL;
	}

	info = spi_nor_ids;
	for (; info->name; info++) {
		if (info->id_len) {
			if (!rt_memcmp(info->id, id, info->id_len))
				return info;
		}
	}

	AX_LOG_ERROR("unrecognized JEDEC id bytes: %02x, %02x, %02x\n", id[0], id[1], id[2]);
	return NULL;
}
#endif

#if 0
static int spinor_unlock_block()
{
	int ret;
	u8 val;

	ret = read_sr(SPINOR_OP_RDSR, &val);
	if (ret < 0)
		return ret;

	write_enable();
	val &= ~(SR_BP0 | SR_BP1 | SR_BP2 | SR_TB);
	write_sr(SPINOR_OP_WRSR, val);

	ret = spinor_wait_till_ready();
	if (ret)
		return ret;

	return 0;
}

int spinor_write(u32 addr, u32 len, const u8 *buf)
{
	u32 page_offset, page_remain, i;
	int ret;

	for (i = 0; i < len; ) {
		int written;
		u32 offset = addr + i;

		page_offset = offset & (SPINOR_PAGE_SIZE - 1);
		/* the size of data remaining on the first page */
		page_remain = (SPINOR_PAGE_SIZE - page_offset) < (len - i) ? \
		(SPINOR_PAGE_SIZE - page_offset) : (len - i);

		write_enable();
		ret = spinor_write_data(offset, page_remain, buf + i);
		if (ret < 0)
			goto write_err;
		written = ret;

		ret = spinor_wait_till_ready();
		if (ret < 0)
			goto write_err;
		i += written;
	}

write_err:
	return ret;
}
#endif
static int spinor_reset()
{
	int ret;
	ret = spinor_write_reg(SPINOR_OP_RSTEN, NULL, 0);
	if (ret < 0)
		return ret;
	ret = spinor_write_reg(SPINOR_OP_RESET, NULL, 0);
	if (ret < 0)
		return ret;

	/* bug fix: RESET cmd needs an recover time */
	// udelay(100);
	ax_udelay(100);

	return 0;
}

int spinor_quad_enable(u8 enable)
{
	int ret;
	u8 val;

	if (!spinor_qe_cfg_parsred)
		return 0;

	spinor_qmode =  (enable ? 1 : 0);
	ret = read_sr(spinor_qe_rdsr, &val);
	if (ret < 0)
		return ret;

	if (enable) {
		if (val & (1 << spinor_qe_bit))
			return 0;
		val |= 1 << spinor_qe_bit;
	} else {
		if (!(val & (1 << spinor_qe_bit)))
			return 0;
		val &= ~(1 << spinor_qe_bit);
	}

#ifdef SPI_NOR_WRITE_ENABLE_FOR_VOLATILE_STATUS_REGISTER
	write_enable_volatile();
#else
	write_enable();
#endif
	write_sr(spinor_qe_wrsr, val);
	ret = spinor_wait_till_ready();
	if (ret)
		return ret;

	return 0;
}

int spinor_read(u32 addr, u32 len, u8 *buf)
{
	return spinor_read_data(addr, len, buf);
}

#ifdef SPI_NOR_ADDR_4BYTE
static u8 spi_nor_convert_opcode(u8 opcode, const u8 table[][2], u32 size)
{
	u32 i;

	for (i = 0; i < size; i++)
		if (table[i][0] == opcode)
			return table[i][1];

	/* No conversion found, keep input op code. */
	return opcode;
}

static u8 spi_nor_convert_3to4_read(u8 opcode)
{
	static const u8 spi_nor_3to4_read[][2] = {
		{ SPINOR_OP_READ,	SPINOR_OP_READ_4B },
		{ SPINOR_OP_READ_FAST,	SPINOR_OP_READ_FAST_4B },
		{ SPINOR_OP_READ_QUAD,	SPINOR_OP_READ_1_1_4_4B },
	};

	return spi_nor_convert_opcode(opcode, spi_nor_3to4_read,
				      ARRAY_SIZE(spi_nor_3to4_read));
}

static u8 spi_nor_convert_3to4_program(u8 opcode)
{
	static const u8 spi_nor_3to4_program[][2] = {
		{ SPINOR_OP_PP,		SPINOR_OP_PP_4B },
		{ SPINOR_OP_PP_QUAD,	SPINOR_OP_PP_1_1_4_4B },
	};

	return spi_nor_convert_opcode(opcode, spi_nor_3to4_program,
				      ARRAY_SIZE(spi_nor_3to4_program));
}

static u8 spi_nor_convert_3to4_erase(u8 opcode)
{
	static const u8 spi_nor_3to4_erase[][2] = {
		{ SPINOR_OP_BE_4K,	SPINOR_OP_BE_4K_4B },
		{ SPINOR_OP_BE_32K,	SPINOR_OP_BE_32K_4B },
		{ SPINOR_OP_BE_64K,	SPINOR_OP_BE_64K_4B },
	};

	return spi_nor_convert_opcode(opcode, spi_nor_3to4_erase,
				      ARRAY_SIZE(spi_nor_3to4_erase));
}

static void spi_nor_set_4byte_opcodes(const struct flash_info *info)
{
	AX_LOG_NOTICE("JEDEC_MFR=0x%x, JEDEC_ID=0x%x, size=0x%x, addr_%dB, default read_opcode=0x%x",
		JEDEC_MFR(info), JEDEC_ID(info), spinor_size, spinor_addr_width, spinor_read_opcode);
	/* Do some manufacturer fixups first */
	switch (JEDEC_MFR(info)) {
	case SNOR_MFR_SPANSION:
		/* No small sector erase for 4-byte command set */
		spinor_erase_opcode = SPINOR_OP_BE_64K;
		spinor_erasesize = info->sector_size;
		break;

	default:
		break;
	}

	spinor_read_opcode = spi_nor_convert_3to4_read(spinor_read_opcode);
	spinor_program_opcode = spi_nor_convert_3to4_program(spinor_program_opcode);
	spinor_erase_opcode = spi_nor_convert_3to4_erase(spinor_erase_opcode);
	AX_LOG_NOTICE("convert read_opcode=0x%x", spinor_read_opcode);
}

/* Enable/disable 4-byte addressing mode. */
static int set_4byte(const struct flash_info *info, int enable)
{
	int status;
	bool need_wren = false;
	u8 cmd;

	AX_LOG_NOTICE("JEDEC_MFR=0x%x, JEDEC_ID=0x%x, size=0x%x, addr_%dB, read_opcode=0x%x",
		JEDEC_MFR(info), JEDEC_ID(info), spinor_size, spinor_addr_width, spinor_read_opcode);
	switch (JEDEC_MFR(info)) {
	case SNOR_MFR_ST:
	case SNOR_MFR_MICRON:
		/* Some Micron need WREN command; all will accept it */
		need_wren = true;
	case SNOR_MFR_MACRONIX:
	case SNOR_MFR_WINBOND:
		if (need_wren)
			write_enable();

		cmd = enable ? SPINOR_OP_EN4B : SPINOR_OP_EX4B;
		status = spinor_write_reg(cmd, NULL, 0);
		if (need_wren)
			write_disable();

		if (!status && !enable &&
		    JEDEC_MFR(info) == SNOR_MFR_WINBOND) {
			/*
			 * On Winbond W25Q256FV, leaving 4byte mode causes
			 * the Extended Address Register to be set to 1, so all
			 * 3-byte-address reads come from the second 16M.
			 * We must clear the register to enable normal behavior.
			 */
			write_enable();
			spinor_cmd_buf[0] = 0;
			spinor_write_reg(SPINOR_OP_WREAR, spinor_cmd_buf, 1);
			write_disable();
		}

		return status;
	default:
		/* Spansion style */
		spinor_cmd_buf[0] = enable << 7;
		return spinor_write_reg(SPINOR_OP_BRWR, spinor_cmd_buf, 1);
	}
}
#endif

int spinor_init(u32 clk, u32 buswidth)
{
	int ret;
#ifdef SPI_NOR_ADDR_4BYTE
	const struct flash_info *info = NULL;
#endif
	u32 quad_en = (buswidth == 4) ? 1 : 0;

	if (spinor_initialized[curr_cs])
		return 0;

	switch (clk) {
	case SPINOR_DEFAULT_CLK:
	default:
		ssi_rx_sample_delay = spinor_rx_sample_delay[0];
		ssi_phy_setting = spinor_phy_setting[0];
		break;

	case SPINOR_25M_CLK:
		ssi_rx_sample_delay = spinor_rx_sample_delay[1];
		ssi_phy_setting = spinor_phy_setting[1];
		break;

	case SPINOR_50M_CLK:
		ssi_rx_sample_delay = spinor_rx_sample_delay[2];
		ssi_phy_setting = spinor_phy_setting[2];
		break;
	}
	spi_hw_init(clk);
	ret = spinor_reset();
	if (ret < 0)
	{
		return ret;
	}
#ifdef SPI_NOR_ADDR_4BYTE
	info = spinor_read_id();
	if (info) {
		spinor_erasesize = info->sector_size;
		spinor_size = info->sector_size * info->n_sectors;
	}
#endif

#if 0
	ret = spinor_unlock_block();
	if (ret < 0)
		return ret;
#endif

	if (quad_en) {
		ret = spinor_quad_enable(quad_en);
		if (ret < 0)
		{
			return ret;
		}
#ifdef SPI_NOR_ADDR_4BYTE
		spinor_read_opcode = SPINOR_OP_READ_QUAD;
		spinor_program_opcode = SPINOR_OP_PP_QUAD;
#endif
	}
#ifdef SPI_NOR_ADDR_4BYTE
	if (spinor_size > (16 * 1024 * 1024)) {
		/* enable 4-byte addressing if the device exceeds 16MiB */
		spinor_addr_width = 4;
		if (JEDEC_MFR(info) == SNOR_MFR_SPANSION ||
		    info->flags & SPI_NOR_4B_OPCODES)
			spi_nor_set_4byte_opcodes(info);
	} else {
		spinor_addr_width = 3;
	}

	if (spinor_addr_width == 4 &&
	    (JEDEC_MFR(info) != SNOR_MFR_SPANSION) &&
	    !(info->flags & SPI_NOR_4B_OPCODES)) {
		set_4byte(info, 1);
	}
#endif
	current_bus_width = buswidth;
	current_clock = clk;

	switch (clk) {
	case SPINOR_DEFAULT_CLK:
	default:
		break;

	case SPINOR_25M_CLK:
	case SPINOR_50M_CLK:
		break;
	}
	spinor_initialized[curr_cs] = 1;

	return 0;
}

