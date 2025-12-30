// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2000-2005, DENX Software Engineering
 *		Wolfgang Denk <wd@denx.de>
 * Copyright (C) Procsys. All rights reserved.
 *		Mushtaq Khan <mushtaq_k@procsys.com>
 *			<mushtaqk_921@yahoo.co.in>
 * Copyright (C) 2008 Freescale Semiconductor, Inc.
 *		Dave Liu <daveliu@freescale.com>
 */

#include <common.h>
#ifdef AX_SATA_BIST_SUPPORT
#include <asm/io.h>
#endif
#include <ahci.h>
#include <dm.h>
#include <command.h>
#include <part.h>
#include <sata.h>
#include <dm/device-internal.h>
#include <dm/uclass-internal.h>

static int sata_curr_device = -1;

int sata_remove(int devnum)
{
#ifdef CONFIG_AHCI
	struct udevice *dev;
	int rc;

	blk_unbind_all(IF_TYPE_SATA);

	rc = uclass_find_device(UCLASS_AHCI, devnum, &dev);
	if (!rc && !dev)
		rc = uclass_find_first_device(UCLASS_AHCI, &dev);
	if (rc || !dev) {
		printf("Cannot find SATA device %d (err=%d)\n", devnum, rc);
		return CMD_RET_FAILURE;
	}

	rc = device_remove(dev, DM_REMOVE_NORMAL);
	if (rc) {
		printf("Cannot remove SATA device '%s' (err=%d)\n", dev->name,
		       rc);
		return CMD_RET_FAILURE;
	}

	return 0;
#else
	return sata_stop();
#endif
}

int sata_probe(int devnum)
{
#ifdef CONFIG_AHCI
	struct udevice *dev;
	int rc;

	rc = uclass_get_device(UCLASS_AHCI, devnum, &dev);
	if (rc)
		rc = uclass_find_first_device(UCLASS_AHCI, &dev);
	if (rc) {
		printf("Cannot probe SATA device %d (err=%d)\n", devnum, rc);
		return CMD_RET_FAILURE;
	}
	if (!dev) {
		printf("No SATA device found!\n");
		return CMD_RET_FAILURE;
	}
	rc = sata_scan(dev);
	if (rc) {
		printf("Cannot scan SATA device %d (err=%d)\n", devnum, rc);
		return CMD_RET_FAILURE;
	}

	return 0;
#else
	return sata_initialize() < 0 ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
#endif
}

#ifdef AX_SATA_BIST_SUPPORT
#define HBA_BASE_ADDRESS 0x31000000
static void ax_reg_writel(ulong reg, u32 val)
{
	printf("write reg addr: 0x%lx, data: 0x%x\n", reg, val);
	writel(val, reg);
}

static u32 ax_reg_readl(ulong reg)
{
	u32 val = readl(reg);
	printf("read reg addr: 0x%lx, data: 0x%x\n", reg, val);
	return val;
}

u32 port_reg_offset(int port_num)
{
	if ((port_num < 0) || (port_num > 3)) {
		printf("%s: input port_num %d error\n", __func__, port_num);
	}
	return (HBA_BASE_ADDRESS + 0x100 + 0x80 * port_num);
}

void hba_reset(void)
{
	printf("%s: start\n", __func__);
	ax_reg_writel((HBA_BASE_ADDRESS + HOST_CTL), HOST_RESET);
	printf("%s: done\n", __func__);
}

void hba_init(void)
{
	int i;

	printf("%s: start\n", __func__);
	ax_reg_readl(HBA_BASE_ADDRESS + HOST_CAP);
	ax_reg_writel((HBA_BASE_ADDRESS + HOST_CAP), 0);
	ax_reg_readl(HBA_BASE_ADDRESS + HOST_CAP);


	ax_reg_writel((HBA_BASE_ADDRESS + HOST_PORTS_IMPL), 0xf);
	ax_reg_readl(HBA_BASE_ADDRESS + HOST_PORTS_IMPL);

	ax_reg_readl(HBA_BASE_ADDRESS + HOST_BISTCR);
	for (i = 0; i < 4; i++) {
		ax_reg_readl(port_reg_offset(i) + PORT_SCR_CTL);
		ax_reg_writel(port_reg_offset(i) + PORT_SCR_CTL, 0x1);
		ax_reg_writel(port_reg_offset(i) + PORT_SCR_CTL, 0x0);
	}
	printf("%s: done\n", __func__);
}

void bist_proc(int port_no, u8 pattern)
{
	u32 val;

	printf("%s: start\n", __func__);
	val = ax_reg_readl(HBA_BASE_ADDRESS + HOST_TESTR);
	val &= ~(0x7 << 16);
	val |= (port_no << 16);
	ax_reg_writel(HBA_BASE_ADDRESS + HOST_TESTR, val);

	val = ax_reg_readl(HBA_BASE_ADDRESS + HOST_BISTCR);
	val &= ~(0xf << 0);
	val |= (pattern << 0) | (1 << 18);
	ax_reg_writel(HBA_BASE_ADDRESS + HOST_BISTCR, val);
	printf("%s: done\n", __func__);
}
#endif
static int do_sata(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int rc = 0;

	if (argc >= 2) {
		int devnum = 0;

		if (argc == 3)
			devnum = (int)simple_strtoul(argv[2], NULL, 10);
		if (!strcmp(argv[1], "stop"))
			return sata_remove(devnum);

		if (!strcmp(argv[1], "init")) {
			if (sata_curr_device != -1) {
				rc = sata_remove(devnum);
				if (rc)
					return rc;
			}

			return sata_probe(devnum);
		}
#ifdef AX_SATA_BIST_SUPPORT
		if (!strcmp(argv[1], "bist")) {
			int pattern;
			int port = 0;

			if (argc >= 3)
				pattern = (int)simple_strtoul(argv[2], NULL, 10);
			else
				return -1;

			if (argc == 4)
				port = (int)simple_strtoul(argv[3], NULL, 10);

			printf("HW BIST pattern %d, port %d\n", pattern, port);
			hba_reset();
			hba_init();
			bist_proc(port, (u8)pattern);
			return 0;
		}
#endif
	}

	/* If the user has not yet run `sata init`, do it now */
	if (sata_curr_device == -1) {
		rc = sata_probe(0);
		if (rc)
			return rc;
		sata_curr_device = 0;
	}

	return blk_common_cmd(argc, argv, IF_TYPE_SATA, &sata_curr_device);
}

U_BOOT_CMD(
	sata, 5, 1, do_sata,
	"SATA sub system",
	"init - init SATA sub system\n"
	"sata stop [dev] - disable SATA sub system or device\n"
	"sata info - show available SATA devices\n"
	"sata device [dev] - show or set current device\n"
	"sata part [dev] - print partition table\n"
	"sata read addr blk# cnt\n"
	"sata write addr blk# cnt"
);
