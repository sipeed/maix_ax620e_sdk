// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 exceet electronics GmbH
 *
 * Authors:
 *	Frieder Schrempf <frieder.schrempf@exceet.de>
 *	Boris Brezillon <boris.brezillon@bootlin.com>
 */

#ifndef __UBOOT__
#include <malloc.h>
#include <linux/device.h>
#include <linux/kernel.h>
#endif
#include <linux/mtd/spinand.h>

#define SPINAND_MFR_WINBOND		0xEF
#define SPINAND_MFR_XTXTECH		0x0B
#define WINBOND_STATUS_ECC_HAS_BITFLIPS_T	(3 << 4)

#define WINBOND_CFG_BUF_READ		BIT(3)

#define SUPPORT_W25N02KW
#define SUPPORT_W25N02KV

static SPINAND_OP_VARIANTS(read_cache_variants,
		//SPINAND_PAGE_READ_FROM_CACHE_QUADIO_OP(0, 2, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_DUALIO_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

static SPINAND_OP_VARIANTS(write_cache_variants,
		SPINAND_PROG_LOAD_X4(true, 0, NULL, 0),
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_variants,
		SPINAND_PROG_LOAD_X4(false, 0, NULL, 0),
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

static int w25m02gv_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 8;
	region->length = 8;

	return 0;
}

static int w25m02gv_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 6;

	return 0;
}

static const struct mtd_ooblayout_ops w25m02gv_ooblayout = {
	.ecc = w25m02gv_ooblayout_ecc,
	.rfree = w25m02gv_ooblayout_free,
};

static int w25n02jw_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 12;
	region->length = 4;

	return 0;
}

static int w25n02jw_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 10;

	return 0;
}

static const struct mtd_ooblayout_ops w25n02jw_ooblayout = {
	.ecc = w25n02jw_ooblayout_ecc,
	.rfree = w25n02jw_ooblayout_free,
};

#ifndef SUPPORT_W25N02KW
static int w25n01gw_ooblayout_ecc(struct mtd_info *mtd, int section,
				 struct mtd_oob_region *region)
{
   if (section > 3)
	   return -ERANGE;

   region->offset = (16 * section) + 8;
   region->length = 8;

   return 0;
}

static int w25n01gw_ooblayout_free(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
   if (section > 3)
	   return -ERANGE;

   region->offset = (16 * section) + 2;
   region->length = 6;

   return 0;
}

static const struct mtd_ooblayout_ops w25n01gw_ooblayout = {
	.ecc = w25n01gw_ooblayout_ecc,
	.rfree = w25n01gw_ooblayout_free,
};
#endif

static int w25n02kw_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 64;
	region->length = 13;

	return 0;
}

static int w25n02kw_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 14;

	return 0;
}

static const struct mtd_ooblayout_ops w25n02kw_ooblayout = {
	.ecc = w25n02kw_ooblayout_ecc,
	.rfree = w25n02kw_ooblayout_free,
};

static int w25m02gv_select_target(struct spinand_device *spinand,
				  unsigned int target)
{
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(0xc2, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_OUT(1,
							spinand->scratchbuf,
							1));

	*spinand->scratchbuf = target;
	return spi_mem_exec_op(spinand->slave, &op);
}

static int w25n02kv_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = 64 + (16 * section);
	region->length = 13;

	return 0;
}

static int w25n02kv_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 14;

	return 0;
}

static const struct mtd_ooblayout_ops w25n02kv_ooblayout = {
	.ecc = w25n02kv_ooblayout_ecc,
	.rfree = w25n02kv_ooblayout_free,
};

static int w25n02kv_ecc_get_status(struct spinand_device *spinand,
				   u8 status)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	u8 mbf = 0;
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(0x30, &mbf);

	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	case STATUS_ECC_HAS_BITFLIPS:
	case WINBOND_STATUS_ECC_HAS_BITFLIPS_T:
		/*
		 * Let's try to retrieve the real maximum number of bitflips
		 * in order to avoid forcing the wear-leveling layer to move
		 * data around if it's not necessary.
		 */
		if (spi_mem_exec_op(spinand->slave, &op))
			return nand->eccreq.strength;

		mbf >>= 4;

		if (WARN_ON(mbf > nand->eccreq.strength || !mbf))
			return nand->eccreq.strength;

		return mbf;

	default:
		break;
	}

	return -EINVAL;
}

#ifdef SUPPORT_W25N02KW
//#define SUPPORT_W25N02KW_BLOCK_PROT
#ifdef SUPPORT_W25N02KW_BLOCK_PROT
#define SR1_TB_BIT_OFFSET (2)
#define SR1_BP_BIT_OFFSET (3)
#define SR1_BP_BIT_MAST   (0xf << SR1_BP_BIT_OFFSET)

enum prot_array {
	PROT_NONE = 0,
	PROT_UPPER_1_512 = 1,
	PROT_UPPER_1_256 = 2,
	PROT_UPPER_1_128 = 3,
	PROT_UPPER_1_64 = 4,
	PROT_UPPER_1_32 = 5,
	PROT_UPPER_1_16 = 6,
	PROT_UPPER_1_8 = 7,
	PROT_UPPER_1_4 = 8,
	PROT_UPPER_1_2 = 9,
	PROT_ALL = 10,
	PROT_LOWER_1_512 = 11,
	PROT_LOWER_1_256 = 12,
	PROT_LOWER_1_128 = 13,
	PROT_LOWER_1_64 = 14,
	PROT_LOWER_1_32 = 15,
	PROT_LOWER_1_16 = 16,
	PROT_LOWER_1_8 = 17,
	PROT_LOWER_1_4 = 18,
	PROT_LOWER_1_2 = 19,
};

static u8 w25n02kw_prot_array_cfg(enum prot_array cfg)
{
	u8 val;

	switch (cfg) {
	case PROT_LOWER_1_512:
	case PROT_UPPER_1_512:
		val = (0x1 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_LOWER_1_256:
	case PROT_UPPER_1_256:
		val = (0x2 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_LOWER_1_128:
	case PROT_UPPER_1_128:
		val = (0x3 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_LOWER_1_64:
	case PROT_UPPER_1_64:
		val = (0x4 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_LOWER_1_32:
	case PROT_UPPER_1_32:
		val = (0x5 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_LOWER_1_16:
	case PROT_UPPER_1_16:
		val = (0x6 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_LOWER_1_8:
	case PROT_UPPER_1_8:
		val = (0x7 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_LOWER_1_4:
	case PROT_UPPER_1_4:
		val = (0x8 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_LOWER_1_2:
	case PROT_UPPER_1_2:
		val = (0x9 << SR1_BP_BIT_OFFSET);
		break;

	case PROT_ALL:
		val = (0xf << SR1_BP_BIT_OFFSET);
		break;

	case PROT_NONE:
	default:
		val = 0;
		break;
	}

	if ((cfg >= PROT_LOWER_1_512) && (cfg <= PROT_LOWER_1_2))
		val |= (1 << SR1_TB_BIT_OFFSET);

	return val;
}

static int w25n02kw_block_prot_cfg(struct spinand_device *spinand, enum prot_array cfg)
{
	u8 sr1;
	struct spi_mem_op op_getsr1 = SPINAND_GET_FEATURE_OP(REG_BLOCK_LOCK, &sr1);
	struct spi_mem_op op_setsr1 = SPINAND_SET_FEATURE_OP(REG_BLOCK_LOCK, &sr1);
	int ret;

	ret = spi_mem_exec_op(spinand->slave, &op_getsr1);
	if (ret)
		return ret;
	dev_info(&spinand->slave->dev, "read reg 0x%x, val 0x%x\n", REG_BLOCK_LOCK, sr1);
	sr1 &= ~((1 << SR1_TB_BIT_OFFSET) | SR1_BP_BIT_MAST);
	sr1 |= w25n02kw_prot_array_cfg(cfg);
	dev_info(&spinand->slave->dev, "write reg 0x%x, val 0x%x\n", REG_BLOCK_LOCK, sr1);
	ret = spi_mem_exec_op(spinand->slave, &op_setsr1);

	return ret;
}
#endif
#endif

static int w25n01kv_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (8 * section) + 64;
	region->length = 7;

	return 0;
}

static int w25n01kv_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = 2;
	region->length = 62;

	return 0;
}

static const struct mtd_ooblayout_ops w25n01kv_ooblayout = {
	.ecc = w25n01kv_ooblayout_ecc,
	.rfree = w25n01kv_ooblayout_free,
};

static int w25n01kv_ecc_get_status(struct spinand_device *spinand,
				   u8 status)
{
	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	case STATUS_ECC_HAS_BITFLIPS:
	default:
		return 4;
	}

	return -EINVAL;
}


static const struct spinand_info winbond_spinand_table[] = {
	SPINAND_INFO("W25M02GV", 0xAB,
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 2),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL),
		     SPINAND_SELECT_TARGET(w25m02gv_select_target)),
#ifndef SUPPORT_W25N02KV
	SPINAND_INFO("W25N01GV", 0xAA,
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL)),
#else
	SPINAND_INFO("W25N02KV", 0xAA,
		     NAND_MEMORG(1, 2048, 128, 64, 2048, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&w25n02kv_ooblayout, w25n02kv_ecc_get_status)),
#endif
	SPINAND_INFO("W25N01KV", 0xAE,
		     NAND_MEMORG(1, 2048, 96, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(4, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&w25n01kv_ooblayout, w25n01kv_ecc_get_status)),
	SPINAND_INFO("W25N02JW", 0xBF,
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 2, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&w25n02jw_ooblayout, NULL)),
#ifndef SUPPORT_W25N02KW
	SPINAND_INFO("W25N01GW", 0xBA,
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&w25n01gw_ooblayout, NULL)),
#else
	SPINAND_INFO("W25N02KW", 0xBA,
		     NAND_MEMORG(1, 2048, 128, 64, 1024, 1, 2, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&w25n02kw_ooblayout, NULL)),
#endif
	SPINAND_INFO("XT26G01C", 0x11,
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL)),
};

/**
 * winbond_spinand_detect - initialize device related part in spinand_device
 * struct if it is a Winbond device.
 * @spinand: SPI NAND device structure
 */
static int winbond_spinand_detect(struct spinand_device *spinand)
{
	u8 *id = spinand->id.data;
	int ret;

	/*
	 * Winbond SPI NAND read ID need a dummy byte,
	 * so the first byte in raw_id is dummy.
	 */
	if (id[1] != SPINAND_MFR_WINBOND && id[1] != SPINAND_MFR_XTXTECH)
		return 0;

	ret = spinand_match_and_init(spinand, winbond_spinand_table,
				     ARRAY_SIZE(winbond_spinand_table), id[2]);
	if (ret)
		return ret;

	return 1;
}

static int winbond_spinand_init(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;
#ifdef SUPPORT_W25N02KW_BLOCK_PROT
	bool block_prot_support = false;

	if (spinand->id.data[2] == 0xBA)
		block_prot_support = true;
#endif

	/*
	 * Make sure all dies are in buffer read mode and not continuous read
	 * mode.
	 */
	for (i = 0; i < nand->memorg.ntargets; i++) {
		spinand_select_target(spinand, i);
		spinand_upd_cfg(spinand, WINBOND_CFG_BUF_READ,
				WINBOND_CFG_BUF_READ);
#ifdef SUPPORT_W25N02KW_BLOCK_PROT
		if (block_prot_support)
			w25n02kw_block_prot_cfg(spinand, PROT_LOWER_1_128);
#endif
	}

	return 0;
}

static const struct spinand_manufacturer_ops winbond_spinand_manuf_ops = {
	.detect = winbond_spinand_detect,
	.init = winbond_spinand_init,
};

const struct spinand_manufacturer winbond_spinand_manufacturer = {
	.id = SPINAND_MFR_WINBOND,
	.name = "Winbond",
	.ops = &winbond_spinand_manuf_ops,
};
