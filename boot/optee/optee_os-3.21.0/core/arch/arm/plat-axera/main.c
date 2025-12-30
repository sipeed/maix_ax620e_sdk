// SPDX-License-Identifier: BSD-2-Clause
/*
 *
 */

#include <console.h>
#include <drivers/gic.h>
#include <drivers/serial8250_uart.h>
#include <kernel/boot.h>
#include <kernel/interrupt.h>
#include <kernel/panic.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <trace.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <tee/entry_fast.h>
#include <tee/entry_std.h>
#include <kernel/tee_common_otp.h>
#include <drivers/driver_efuse.h>
#include <utee_defines.h>
#include <crypto/crypto.h>

#define EFUSE_BONDOPT_BLK	13
#define EFUSE_HUK_BLK_START	32
#define EFUSE_SPACE_SHIFT	24
#define HUK_SIZE_IN_U8		HW_UNIQUE_KEY_LENGTH
#define HUK_SIZE_IN_U32		(HW_UNIQUE_KEY_LENGTH / 4)
#define EFSC_SHADOW_0		0x58

register_phys_mem_pgdir(MEM_AREA_IO_NSEC,
			ROUNDDOWN(CONSOLE_UART_BASE, CORE_MMU_PGDIR_SIZE),
			CORE_MMU_PGDIR_SIZE);

register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			ROUNDDOWN(GIC_BASE, CORE_MMU_PGDIR_SIZE),
			CORE_MMU_PGDIR_SIZE);

register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			ROUNDDOWN(GIC_BASE + GICD_OFFSET, CORE_MMU_PGDIR_SIZE),
			CORE_MMU_PGDIR_SIZE);

register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			ROUNDDOWN(EFUSE0_BASE, CORE_MMU_PGDIR_SIZE),
			CORE_MMU_PGDIR_SIZE);

register_phys_mem_pgdir(MEM_AREA_IO_SEC,
			ROUNDDOWN(COMMON_SYS_BASE, CORE_MMU_PGDIR_SIZE),
			CORE_MMU_PGDIR_SIZE);
static struct gic_data gic_data;
static struct serial8250_uart_data console_data;

void console_init(void)
{
	serial8250_uart_init(&console_data, CONSOLE_UART_BASE,
			     CONSOLE_UART_CLK_IN_HZ, CONSOLE_BAUDRATE);
	register_serial_console(&console_data.chip);
}

void main_init_gic(void)
{
	gic_init_base_addr(&gic_data, GIC_BASE + GICC_OFFSET,
			   GIC_BASE + GICD_OFFSET);

	itr_init(&gic_data.chip);
}

void itr_core_handler(void)
{
	gic_it_handle(&gic_data);
}

int tee_otp_get_die_id(uint8_t *buffer, size_t len)
{
	static uint64_t uid;
	size_t i;
	static uint64_t uid_read = 0;
	vaddr_t efuse0_base;
	uint64_t tmp;
	char *ptr = (char *)&uid;
	if(!uid_read) {
		efuse0_base = (vaddr_t)phys_to_virt(EFUSE0_BASE, MEM_AREA_IO_SEC, EFUSE0_SZ);
		uid = io_read32(efuse0_base + EFSC_SHADOW_0);
		tmp = io_read32(efuse0_base + EFSC_SHADOW_0 + 4);
		tmp <<= 32;
		uid |= tmp;
		uid_read = 1;
	}
	for (i = 0; i < len; i++) {
		buffer[i] = ptr[i % 8];
	}

	return 0;
}

static int32_t ax_huk_read(uint8_t *huk, uint32_t count)
{
	int ret, i;
	uint32_t val;
	uint32_t rand_data[HUK_SIZE_IN_U32];

	ret = ax_efuse_init();
	if (ret) {
		EMSG("ax efuse init failed");
		return -1;
	}

	ret = ax_efuse_read(EFUSE_BONDOPT_BLK, &val);
	if (ret) {
		EMSG("ax efuse read EFUSE_BONDOPT_BLK failed");
		goto efuse_deinit_ret;
	} else if (!(val & (1 << EFUSE_SPACE_SHIFT))) {
		ret = -1;
		EMSG("ax efuse EFUSE_BONDOPT_BLK:0x%08x, EFUSE_SPACE bit isn't set, cannot access expected blks", val);
		goto efuse_deinit_ret;
	}

	for (i = 0; i < HUK_SIZE_IN_U32; i++) {
		ret = ax_efuse_read(EFUSE_HUK_BLK_START + i , &rand_data[i]);
		if (ret) {
			EMSG("ax efuse read efuse blk[%d] failed", EFUSE_HUK_BLK_START + i);
			goto efuse_deinit_ret;
		}
	}
	memcpy((void *)huk, (void *)rand_data, count);
	/* check whether huk efuse blks are all 0 and give warning */
	for (i = 0; i < HUK_SIZE_IN_U32; i++) {
		if (rand_data[i] != 0)
			break;
	}
	if (i == HUK_SIZE_IN_U32) {
		EMSG("warning HUK efuse blk is empty");
	}

efuse_deinit_ret:
	ax_efuse_deinit();

	return ret;
}

static int ax_huk_init(void)
{
#if 0
	int ret, i;
	uint32_t val;
	uint32_t rand_data[HUK_SIZE_IN_U32];
	uint8_t *p_random_byte = (uint8_t *)rand_data;

	/* step1: set efuse blk13 bit24 efuse space to let cpu access blk32-63 not ce */
	ret = ax_efuse_init();
	if (ret) {
		EMSG("ax efuse init failed");
		return -1;
	}
	ret = ax_efuse_read(EFUSE_BONDOPT_BLK, &val);
	if (ret) {
		EMSG("ax efuse read failed");
		goto efuse_deinit_ret;
	} else if (!(val & (1 << EFUSE_SPACE_SHIFT))) {
		ret = -1;
		EMSG("ax efuse EFUSE_BONDOPT_BLK:0x%08x, EFUSE_SPACE bit isn't set, cannot access expected blks", val);
		goto efuse_deinit_ret;
	}

	/* step2: check whether huk efuse blks are all 0, if not we think huk has been written */
	for (i = 0; i < HUK_SIZE_IN_U32; i++) {
		ret = ax_efuse_read(EFUSE_HUK_BLK_START + i, &val);
		if (ret) {
			EMSG("ax efuse read huk blks failed");
			goto efuse_deinit_ret;
		}
		//EMSG("ax get efuse blk[%d]:0x%08x", EFUSE_HUK_BLK_START + i, val);
		if (val != 0)
			break;
	}
	if (i != HUK_SIZE_IN_U32) {
		ret = 0;
		EMSG("ax huk has been initialized");
		goto efuse_deinit_ret;
	}

	/* step3: get random byte */
	ret = crypto_rng_read((void *)p_random_byte, HUK_SIZE_IN_U8);
	if (ret != TEE_SUCCESS) {
		ret = -1;
		EMSG("ax_huk_init crypto_rng_read return error");
		goto efuse_deinit_ret;
	}

	/* step4: write to efuse blk32-35(4blks/128bit) */
	for (i = 0; i < HUK_SIZE_IN_U32; i++) {
		ret = ax_efuse_write(EFUSE_HUK_BLK_START + i, rand_data[i]);
		if (ret) {
			EMSG("ax efuse write efuse rand_data[%d] failed", i);
			goto efuse_deinit_ret;
		}
	}
	EMSG("ax huk has been written and initialized");

efuse_deinit_ret:
	ax_efuse_deinit();

	return ret;
#else
	return 0;
#endif
}

#ifdef CFG_HW_UNQ_KEY_REQUEST
#include <kernel/tee_common_otp.h>
TEE_Result tee_otp_get_hw_unique_key(struct tee_hw_unique_key *hwkey)
{
	TEE_Result res;
	int ret = 0;
	static uint8_t hw_unq_key[sizeof(hwkey->data)] __aligned(64);
	static int hw_unq_key_read = 0;

	if(hw_unq_key_read == 0) {
		ret = ax_huk_init();
		if (!ret)
			ret = ax_huk_read(hw_unq_key, sizeof(hwkey->data));
	}
	if (ret < 0) {
		EMSG("\nUnique key is not fetched from the platform.");
		memset(&hwkey->data[0], 0, sizeof(hwkey->data));
		hw_unq_key_read = 1;
		res = TEE_SUCCESS;
	} else {
		memcpy(&hwkey->data[0], hw_unq_key, sizeof(hwkey->data));
		hw_unq_key_read = 1;
		res = TEE_SUCCESS;
	}
	return res;
}
#endif
