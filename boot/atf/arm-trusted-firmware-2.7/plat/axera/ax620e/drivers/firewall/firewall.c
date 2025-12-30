/*
 * Copyright (c) 2013-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <lib/mmio.h>
#include <ddr_sys.h>
#include <npu_sys.h>
#include <ax620e_def.h>

#define ENABLE					1
#define DISABLE					0

#define FIREWALL_REGION_NUM		8

#define REGION_SETTING_REG_SIZE	64	/* each set of region setting regs size */
#define MASTER_SETTING_REG_SIZE	8	/* each set of master setting regs size */

#define MASTER_ID_CA53			0
#define MASTER_ID_EMMC			1
#define MASTER_ID_AXDMA			4
#define MASTER_ID_AXDMA_CMD		5
#define MASTER_ID_GZIPD			6
#define MASTER_ID_FLASH_OTHER	7
#define MASTER_ID_DW_DMA		8
#define MASTER_ID_AXDMA_PER		9
#define MASTER_ID_CE			10
#define MASTER_ID_ISP			20
#define MASTER_ID_NPU			21
#define MASTER_ID_VDEC			16
#define MASTER_ID_VENC			17
#define MASTER_ID_JENC			18
#define MASTER_ID_MM			19
#define MASTER_ID_RISC_V		11
#define MASTER_ID_DBG			12


#define REGION_SETTING_REG(GRP_BASE)			((GRP_BASE) + 0x0)
#define MASTER_SETTING_REG(GRP_BASE)			((GRP_BASE) + 0x1000)
#define CFG_REG(GRP_BASE)						((GRP_BASE) + 0x1400)
#define PORT_ERROR_STATUS_INT_REG(GRP_BASE)		((GRP_BASE) + 0x1800)

#define CHOSEN_MASTER_ID(GRP_BASE)				(MASTER_SETTING_REG(GRP_BASE) + 0x0)
#define IGNORE_MASTER_PROT(GRP_BASE)			(MASTER_SETTING_REG(GRP_BASE) + 0x4)

#define REGION_START(GRP_BASE)					(REGION_SETTING_REG(GRP_BASE) + 0x0)
#define REGION_END(GRP_BASE)					(REGION_SETTING_REG(GRP_BASE) + 0x4)
#define REGION_EN(GRP_BASE)						(REGION_SETTING_REG(GRP_BASE) + 0x8)
#define REGION_PERMITTED_MASTER0(GRP_BASE)		(REGION_SETTING_REG(GRP_BASE) + 0xC)
#define REGION_UPD_PRMTD_MASTER(GRP_BASE)		(REGION_SETTING_REG(GRP_BASE) + 0x24)

#define AUTO_CLK_GATING(GRP_BASE)				(CFG_REG(GRP_BASE) + 0xC)

#define BIT_FIREWALL_AUTO_CLK_GATE_EN	(1 << 0)
#define BIT_FIREWALL_REGION_EN			(1 << 0)


typedef struct {
	uintptr_t grp_base;
	uint8_t mst_id;
	uint8_t mst_index;
	uint8_t mst_ignore;
} fw_maste_param_t;

typedef struct {
	uint32_t region_start_4KB;
	uint32_t region_end_4KB;
	uint32_t permitted_master_other;
	uint32_t permitted_master_npu;
} fw_region_t;

static void firewall_pclk_enable(int enable)
{
	uint32_t val;

	if (enable) {
		val = BIT_PCLK_FW_OTHERS_EB_SET | BIT_PCLK_FW_NPU_EB_SET;
		mmio_write_32(DDR_SYS_CLK_EB_2_SET_ADDR, val);
	} else {
		val = BIT_PCLK_FW_OTHERS_EB_CLR | BIT_PCLK_FW_NPU_EB_CLR;
		mmio_write_32(DDR_SYS_CLK_EB_2_CLR_ADDR, val);
	}
}

static void firewall_region_enable(uintptr_t grp_base, int region, int enable)
{
	if (enable)
		mmio_write_32(REGION_EN(grp_base) + region * REGION_SETTING_REG_SIZE,
				BIT_FIREWALL_REGION_EN);
	else
		mmio_write_32(REGION_EN(grp_base) + region * REGION_SETTING_REG_SIZE, 0);
}

#ifdef FIREWALL_IS_ENABLED
static const fw_maste_param_t fw_mst[] = {
	{FIREWALL_OTHERS_BASE, MASTER_ID_CA53, 0, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_EMMC, 1, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_AXDMA, 2, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_AXDMA_CMD, 3, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_GZIPD, 4, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_FLASH_OTHER, 5, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_DW_DMA, 6, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_AXDMA_PER, 7, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_CE, 8, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_ISP, 9, 0},
	{FIREWALL_NPU_BASE, MASTER_ID_NPU, 0, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_VDEC, 10, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_VENC, 11, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_JENC, 12, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_MM, 13, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_RISC_V, 14, 0},
	{FIREWALL_OTHERS_BASE, MASTER_ID_DBG, 15, 0},
};

static const fw_region_t fw_region[] = {
#ifdef OPTEE_BOOT
	{
		.region_start_4KB = (OPTEE_IMAGE_ADDR - DDR_START_ADDR) >> 12,
		.region_end_4KB = (OPTEE_IMAGE_ADDR + OPTEE_RESERVED_SIZE - OPTEE_SHMEM_SIZE - 1 - DDR_START_ADDR) >> 12,
		.permitted_master_other = 0x101,	/* only cpu&ce are permitted */
		.permitted_master_npu = 0
	 },
#endif
};

static void firewall_init_regions(void)
{
	int i = 0;

	for(i = 0; i < ARRAY_SIZE(fw_region);i++)
	{
		/* set region start and end */
		mmio_write_32(REGION_START(FIREWALL_OTHERS_BASE) + i * REGION_SETTING_REG_SIZE,
						fw_region[i].region_start_4KB);
		mmio_write_32(REGION_START(FIREWALL_NPU_BASE) + i * REGION_SETTING_REG_SIZE,
						fw_region[i].region_start_4KB);
		mmio_write_32(REGION_END(FIREWALL_OTHERS_BASE) + i * REGION_SETTING_REG_SIZE,
						fw_region[i].region_end_4KB);
		mmio_write_32(REGION_END(FIREWALL_NPU_BASE) + i * REGION_SETTING_REG_SIZE,
						fw_region[i].region_end_4KB);

		/* region permitted master */
		mmio_write_32(REGION_PERMITTED_MASTER0(FIREWALL_NPU_BASE) + i * REGION_SETTING_REG_SIZE,
						fw_region[i].permitted_master_npu);
		mmio_write_32(REGION_PERMITTED_MASTER0(FIREWALL_OTHERS_BASE) + i * REGION_SETTING_REG_SIZE,
						fw_region[i].permitted_master_other);

		/* update */
		mmio_write_32(REGION_UPD_PRMTD_MASTER(FIREWALL_NPU_BASE) + i * REGION_SETTING_REG_SIZE, 1);
		mmio_write_32(REGION_UPD_PRMTD_MASTER(FIREWALL_OTHERS_BASE) + i * REGION_SETTING_REG_SIZE, 1);

		/* region en */
		firewall_region_enable(FIREWALL_NPU_BASE, i, ENABLE);
		firewall_region_enable(FIREWALL_OTHERS_BASE, i, ENABLE);
	}
}


static void firewall_init_masters(void)
{
	int i = 0;

	for(i = 0; i < ARRAY_SIZE(fw_mst); i++)
	{
		mmio_write_32(CHOSEN_MASTER_ID(fw_mst[i].grp_base) + fw_mst[i].mst_index * MASTER_SETTING_REG_SIZE,
						fw_mst[i].mst_id);
		mmio_write_32(IGNORE_MASTER_PROT(fw_mst[i].grp_base) + fw_mst[i].mst_index * MASTER_SETTING_REG_SIZE,
						fw_mst[i].mst_ignore);
	}
}

static void firewall_auto_clk_gateing(int enable)
{
	if (enable) {
		mmio_write_32(AUTO_CLK_GATING(FIREWALL_NPU_BASE),
						BIT_FIREWALL_AUTO_CLK_GATE_EN);
		mmio_write_32(AUTO_CLK_GATING(FIREWALL_OTHERS_BASE),
						BIT_FIREWALL_AUTO_CLK_GATE_EN);
	} else {
		mmio_write_32(AUTO_CLK_GATING(FIREWALL_NPU_BASE), 0);
		mmio_write_32(AUTO_CLK_GATING(FIREWALL_OTHERS_BASE), 0);
	}
}
#endif

void firewall_config(void)
{
#ifdef FIREWALL_IS_ENABLED
	firewall_pclk_enable(ENABLE);

	firewall_init_masters();

	//region config
	firewall_init_regions();

	firewall_auto_clk_gateing(ENABLE);
#else
	int i;

	firewall_pclk_enable(ENABLE);

	for (i = 0; i < FIREWALL_REGION_NUM; i++) {
		firewall_region_enable(FIREWALL_NPU_BASE, i, DISABLE);
		firewall_region_enable(FIREWALL_OTHERS_BASE, i, DISABLE);
	}

	firewall_pclk_enable(DISABLE);
#endif
}
