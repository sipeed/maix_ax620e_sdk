/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <assert.h>
#include <initcall.h>
#include <io.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <trace.h>
#include <kernel/spinlock.h>
#include <drivers/driver_efuse.h>

#define EFSC_WRITE_ENABLE 1

#define EFSC_CE_BUSY_STATUS (0x00)
#define EFSC_INTR_STATUS (0x04)
#define EFSC_INT_RAW (0x08)
#define EFSC_INT_MASK (0x0c)
#define EFSC_INT_MASK_SET (0x10)
#define EFSC_INT_MASK_CLR  (0x14)
#define EFSC_INT_CLR  (0x18)
#define EFSC_INT_CLR_SET  (0x1C)
#define EFSC_INT_CLR_CLR  (0x20)
#define EFSC_PASSWORD  (0x24)
#define EFSC_PASSWORD_SET  (0x28)
#define EFSC_PASSWORD_CLR  (0x2C)
#define EFSC_PGM_VALUE  (0x30)
#define EFSC_PGM_VALUE_SET  (0x34)
#define EFSC_PGM_VALUE_CLR  (0x38)
#define EFSC_APB_CTRL  (0x3C)
#define EFSC_APB_CTRL_SET  (0x40)
#define EFSC_APB_CTRL_CLR  (0x44)
#define EFSC_EFS_START  (0x48)
#define EFSC_EFS_START_SET  (0x4C)
#define EFSC_EFS_START_CLR  (0x50)
#define EFSC_RF_BO_STATUS  (0x54)

#define EFSC_SHADOW_0  (0x58)
#define EFSC_SHADOW_1  (0x5C)
#define EFSC_SHADOW_2  (0x60)
#define EFSC_SHADOW_3  (0x64)
#define EFSC_SHADOW_4  (0x68)
#define EFSC_SHADOW_5  (0x6C)
#define EFSC_SHADOW_6  (0x70)
#define EFSC_SHADOW_7  (0x74)
#define EFSC_SHADOW_8  (0x78)
#define EFSC_SHADOW_9  (0x7C)
#define EFSC_SHADOW_10 (0x80)
#define EFSC_SHADOW_11 (0x84)
#define EFSC_SHADOW_12 (0x88)
#define EFSC_SHADOW_13 (0x8C)
#define EFSC_SHADOW_14 (0x90)
#define EFSC_SHADOW_15 (0x94)
#define EFSC_SHADOW_16 (0x98)
#define EFSC_SHADOW_17 (0x9C)
#define EFSC_SHADOW_18 (0xA0)
#define EFSC_SHADOW_19 (0xA4)
#define EFSC_SHADOW_20 (0xA8)
#define EFSC_SHADOW_21 (0xAC)
#define EFSC_SHADOW_22 (0xB0)
#define EFSC_SHADOW_23 (0xB4)
#define EFSC_SHADOW_24 (0xB8)
#define EFSC_SHADOW_25 (0xBC)
#define EFSC_SHADOW_26 (0xC0)
#define EFSC_SHADOW_27 (0xC4)
#define EFSC_SHADOW_28 (0xC8)
#define EFSC_SHADOW_29 (0xCC)
#define EFSC_SHADOW_30 (0xD0)
#define EFSC_SHADOW_31 (0xD4)
#define EFSC_SHADOW_32 (0xD8)
#define EFSC_SHADOW_33 (0xDC)
#define EFSC_SHADOW_34 (0xE0)
#define EFSC_SHADOW_35 (0xE4)
#define EFSC_SHADOW_36 (0xE8)
#define EFSC_SHADOW_37 (0xEC)
#define EFSC_SHADOW_38 (0xF0)
#define EFSC_SHADOW_39 (0xF4)
#define EFSC_SHADOW_40 (0xF8)
#define EFSC_SHADOW_41 (0xFC)
#define EFSC_SHADOW_42 (0x100)
#define EFSC_SHADOW_43 (0x104)
#define EFSC_SHADOW_44 (0x108)
#define EFSC_SHADOW_45 (0x10C)
#define EFSC_SHADOW_46 (0x110)
#define EFSC_SHADOW_47 (0x114)
#define EFSC_SHADOW_48 (0x118)
#define EFSC_SHADOW_49 (0x11C)
#define EFSC_SHADOW_50 (0x120)
#define EFSC_SHADOW_51 (0x124)
#define EFSC_SHADOW_52 (0x128)
#define EFSC_SHADOW_53 (0x12C)
#define EFSC_SHADOW_54 (0x130)
#define EFSC_SHADOW_55 (0x134)
#define EFSC_SHADOW_56 (0x138)
#define EFSC_SHADOW_57 (0x13C)
#define EFSC_SHADOW_58 (0x140)
#define EFSC_SHADOW_59 (0x144)
#define EFSC_SHADOW_60 (0x148)
#define EFSC_SHADOW_61 (0x14C)
#define EFSC_SHADOW_62 (0x150)
#define EFSC_SHADOW_63 (0x154)

#define EFSC_RF01      (0x158)
#define EFSC_EF23      (0x15C)
#define EFSC_CE_READ_APB_EN  (0x160)
#define EFSC_CE_READ_APB_EB_SET  (0x164)
#define EFSC_CE_READ_APB_EB_CLR  (0x168)
#define EFSC_DUMMY_OUT  (0x16C)
#define EFSC_DUMMY_OUT_SET  (0x170)
#define EFSC_DUMMY_OUT_CLR  (0x174)
#define EFSC_DUMMY_IN       (0x178)
#define EFSC_DUMMY_EN       (0x17C)
#define EFSC_DUMMY_EN_SET   (0x180)
#define EFSC_DUMMY_EN_CLR   (0x184)
#define EFSC_BIT_0_TO_1_ILLEGAL (0x188)
#define EFSC_BIT_1_TO_0_ILLEGAL (0x18C)

#define EFSC_PASS_WD (0x61696370)
#define EFSC_TIMEOUT 10000
#define EFUSE_REG_LEN (0x1000)

#define EFSC0_PROTECT_CE_LOCK0 31
#define EFSC0_PROTECT_BLK0_LOCK0 62
#define EFSC0_PROTECT_BLK0_LOCK1 63

#define EFSC0_BLK_MAX_ID 63

#define AX_EFUSE_PROCESS_PUBLIC __attribute__((visibility("default")))

vaddr_t efuseBase0;
vaddr_t common_base;
static int efuse_flag = 0;
int ax_efuse_init(void)
{
	uint32_t val;

	common_base = (vaddr_t)phys_to_virt(COMMON_SYS_BASE, MEM_AREA_IO_SEC, COMMON_SYS_SZ);
	if (common_base == 0) {
		DMSG("common_base remap failed!\n");
		return -1;
	}
	efuseBase0 = (vaddr_t)phys_to_virt(EFUSE0_BASE, MEM_AREA_IO_SEC, EFUSE0_SZ);
	if (efuseBase0 == 0) {
		DMSG("efuseBase0 remap failed!\n");
		return -1;
	}
	val = io_read32(common_base + 0x30);
	if ((val & (1 << 4)) == 0) {
		io_write32(common_base + 0x34, 1 << 4);
	}
	else
		efuse_flag = 1;

	val = io_read32(efuseBase0 + EFSC_INT_RAW);
	if (val & (1 << 2)) {
		efuseBase0 = 0;
		return -1;
	}
	return 0;
}

int ax_efuse_deinit(void)
{
	if (efuseBase0) {
		efuseBase0 = 0;
	}
	if ((efuse_flag == 1) && common_base) {
		common_base = 0;
		efuse_flag = 0;
	} else {
		io_write32(common_base + 0x38, 1 << 4);
		common_base = 0;
	}
	return 0;
}

int _efuse_read(unsigned long efuseBase, uint32_t blk, uint32_t *data)
{
	*data = io_read32(EFSC_SHADOW_0 + efuseBase + blk * 4);

	return 0;
}

int ax_efuse_read(uint32_t blk, uint32_t *data)
{
	if (blk <= EFSC0_BLK_MAX_ID) {
		return _efuse_read(efuseBase0, blk, data);
	}
	return -1;
}

static int _efuse_write(unsigned long efuseBase, uint32_t blk, uint32_t data)
{
	uint32_t val;
	uint32_t i;
	uint32_t ret = 0;
	val = (blk << 3);
	i = 0;

	/*passwd*/
	io_write32(efuseBase + EFSC_PASSWORD, EFSC_PASS_WD);
	/*pgm_value*/
	io_write32(efuseBase + EFSC_PGM_VALUE, data);
	io_write32(efuseBase + EFSC_APB_CTRL, val);
	/*efs start*/
	io_write32(efuseBase + EFSC_EFS_START, 1);
	/*clr efs start*/
	io_write32(efuseBase + EFSC_EFS_START, 0);
	while (1) {
		if (io_read32(efuseBase + EFSC_INTR_STATUS) & 0x1) {
			break;
		}
		i++;
		if (i > EFSC_TIMEOUT) {
			ret = -1;
			break;
		}
	}
	/*interrupt status clear*/
	io_write32(efuseBase + EFSC_INT_CLR, 0xf);
	/*passwd*/
	io_write32(efuseBase + EFSC_PASSWORD, 0);
	/*clear pgm done*/
	io_write32(efuseBase + EFSC_APB_CTRL, 0);

	return ret;
}

int ax_efuse_write(uint32_t blk, uint32_t data)
{
	if (blk <= EFSC0_BLK_MAX_ID) {
		return _efuse_write(efuseBase0, blk, data);
	}
	return -1;
}

int ax_efuse_blk_protect(uint32_t blk)
{
	uint32_t val;
	uint32_t value;
	ax_efuse_read(13, &value);
	if((value & (1 << 24))) {
		if (blk <= 31) {
			val = (1 << blk);
			return ax_efuse_write(EFSC0_PROTECT_BLK0_LOCK0, val);
		}
		else if (blk >= 32 && blk <= 61) {
			val = 1 << (blk - 32);
			return ax_efuse_write(EFSC0_PROTECT_BLK0_LOCK1, val);
		}
	} else {
		if (blk <= 30) {
			val = (1 << blk);
			return ax_efuse_write(EFSC0_PROTECT_CE_LOCK0, val);
		}
	}
	return -1;
}