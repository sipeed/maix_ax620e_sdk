// SPDXLicenseIdentifier: GPL2.0
/*
 * (C) Copyright 2022 Axera Co., Ltd
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <sysreset.h>
#include <asm/io.h>
#include <linux/err.h>
extern void reboot(void);
int axera_sysreset_request(struct udevice *dev, enum sysreset_t type)
{
	switch (type) {
	case SYSRESET_WARM:
	case SYSRESET_COLD:
		puts("axera_sysreset begin\n");
		reboot();
		break;
	default:
		return EPROTONOSUPPORT;
	}
	return EINPROGRESS;
}

static struct sysreset_ops axera_sysreset = {
	.request	= axera_sysreset_request,
};

U_BOOT_DRIVER(sysreset_axera) = {
	.name	= "axera_sysreset",
	.id	= UCLASS_SYSRESET,
	.ops	= &axera_sysreset,
};
