/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PMU_H
#define PMU_H

#include <pmu_common.h>
#include <lib/mmio.h>
#include <ax620e_def.h>

#define SYS_PWR_STATE_PER_GRP_MAX   8
#define SYS_PWR_STATE_LEN	4
#define SYS_PWR_STATE_MASK	0xF

static int inline pmu_get_module_state(MODULES_ENUM module, unsigned int *state)
{
	int i;
	unsigned int value;
	int ret = -1;

	MODULES_ENUM grp1_modules[SYS_PWR_STATE_PER_GRP_MAX] =
	{MODULE_CPU, MODULE_DDR, MODULE_FLASH, MODULE_ISP, MODULE_MM,
	 MODULE_NPU, MODULE_VPU, MODULE_PERIPH};

	for (i = 0; i < SYS_PWR_STATE_PER_GRP_MAX; i++) {
		if (grp1_modules[i] == module) {
			ret = 0;
			break;
		}
	}
	if (!ret) {
		value = mmio_read_32(PMU_GLB_PWR_STATE_ADDR);
		*state = (value >> (i * SYS_PWR_STATE_LEN)) & SYS_PWR_STATE_MASK;
	}

	return ret;
}

static int inline pmu_module_sleep_en(MODULES_ENUM module, SLP_EN_ENUM en_set_clr)
{
	int ret = 0;
	unsigned long val = 0;

	switch (module) {
 	case MODULE_PERIPH:
	case MODULE_VPU:
	case MODULE_NPU:
	case MODULE_MM:
	case MODULE_ISP:
	case MODULE_FLASH:
	case MODULE_DDR:
	case MODULE_CPU:
		if (SLP_EN_SET == en_set_clr) {
			mmio_write_32(PMU_GLB_SLP_EN_SET_ADDR, (1 << (module - MODULE_GROUP1_START)));
		} else if (SLP_EN_CLR == en_set_clr) {
			mmio_write_32(PMU_GLB_SLP_EN_CLR_ADDR, (1 << (module - MODULE_GROUP1_START)));
		}
 		break;
	default:
		ret = -1;
	}

	if ((SLP_EN_SET == en_set_clr) && (module == MODULE_CPU)) {

		val = mmio_read_32(PMU_GLB_CPU_SYS_SLP_REQ_ADDR);
		val |= BIT_PMU_GLB_CPU_SYS_SLP_REQ_MASK;
		mmio_write_32(PMU_GLB_CPU_SYS_SLP_REQ_ADDR, val);

		/*inorder to use ca53_cfg_rvbaraddr0_h register to set cpu reset addr,
		we must clear below bits here or we must set it to 1.
		*/
		val = mmio_read_32(PMU_GLB_PWR_BYPASS_ADDR);
		PMU_GLB_PWR_BYPASS_CPU_CLR(val);
		//PMU_GLB_PWR_BYPASS_CPU_SET(val);
		mmio_write_32(PMU_GLB_PWR_BYPASS_ADDR, val);
	}

	return ret;
}

static int inline pmu_module_wakeup_set_clr(MODULES_ENUM module, int set)
{
	int ret = 0;

	switch (module) {
 	case MODULE_CPU:
	case MODULE_DDR:
	case MODULE_FLASH:
	case MODULE_ISP:
	case MODULE_MM:
	case MODULE_NPU:
	case MODULE_VPU:
	case MODULE_PERIPH:
		if (set) {
			//mmio_write_32(PMU_GLB_RST_FRC_SW_CLR_ADDR, (1 << (module - MODULE_GROUP1_START)));
			//mmio_write_32(PMU_GLB_RST_FRC_EN_SET_ADDR, (1 << (module - MODULE_GROUP1_START)));
			mmio_write_32(PMU_GLB_WAKUP_SET_ADDR, (1 << (module - MODULE_GROUP1_START)));
		} else {
			mmio_write_32(PMU_GLB_WAKUP_CLR_ADDR, (1 << (module - MODULE_GROUP1_START)));
			//mmio_write_32(PMU_GLB_RST_FRC_SW_SET_ADDR, (1 << (module - MODULE_GROUP1_START)));
			//mmio_write_32(PMU_GLB_RST_FRC_EN_CLR_ADDR, (1 << (module - MODULE_GROUP1_START)));
		}
 		break;
	default:
		ret = -1;
	}

	return ret;
}

static int inline pmu_module_wakeup(MODULES_ENUM module)
{
	unsigned int state = 0x5;
	int ret;
	int i;
	unsigned int value;

	MODULES_ENUM grp1_modules[SYS_PWR_STATE_PER_GRP_MAX] =
	{MODULE_CPU, MODULE_DDR, MODULE_FLASH, MODULE_ISP, MODULE_MM,
	 MODULE_NPU, MODULE_VPU, MODULE_PERIPH};

	ret = pmu_module_wakeup_set_clr(module, 1);
	if (ret)
		goto RET;

	// while (!(ret = pmu_get_module_state(module, &state)) && (state != PWR_STATE_ON));
	// if (ret) {
	// 	goto RET;
	// }
	for (i = 0; i < SYS_PWR_STATE_PER_GRP_MAX; i++) {
		if (grp1_modules[i] == module) {
			ret = 0;
			break;
		}
	}
	while(state != PWR_STATE_ON) {
		value = mmio_read_32(PMU_GLB_PWR_STATE_ADDR);
		state = (value >> (i * SYS_PWR_STATE_LEN)) & SYS_PWR_STATE_MASK;
	}

	/* wake_up_clr */
	ret = pmu_module_wakeup_set_clr(module, 0);

RET:
	return ret;
}

static int inline pmu_get_int_raw_state(MODULES_ENUM module, PWR_STATE_ENUM state)
{
	int ret = 0;
	unsigned int bit_pos;
 	unsigned int val_int_raw = mmio_read_32(PMU_GLB_INT_RAW_ADDR);

	switch(module) {
	case MODULE_CPU:
		bit_pos = 13;
		if (PWR_STATE_OFF == state) {
			bit_pos = bit_pos - 1;
		}
		ret = val_int_raw & (1 << bit_pos);
		break;
	case MODULE_DDR:
		bit_pos = 11;
		if (PWR_STATE_OFF == state) {
			bit_pos = bit_pos - 1;
		}
		ret = val_int_raw & (1 << bit_pos);
		break;
	case MODULE_FLASH:
		bit_pos = 9;
		if (PWR_STATE_OFF == state) {
			bit_pos = bit_pos - 1;
		}
		ret = val_int_raw & (1 << bit_pos);
		break;
	case MODULE_ISP:
		bit_pos = 7;
		if (PWR_STATE_OFF == state) {
			bit_pos = bit_pos - 1;
		}
		ret = val_int_raw & (1 << bit_pos);
		break;
	case MODULE_MM:
		bit_pos = 5;
		if (PWR_STATE_OFF == state) {
			bit_pos = bit_pos - 1;
		}
		ret = val_int_raw & (1 << bit_pos);
		break;
	case MODULE_NPU:
		bit_pos = 3;
		if (PWR_STATE_OFF == state) {
			bit_pos = bit_pos - 1;
		}
		ret = val_int_raw & (1 << bit_pos);
		break;
	case MODULE_VPU:
		bit_pos = 1;
		if (PWR_STATE_OFF == state) {
			bit_pos = bit_pos - 1;
		}
		ret = val_int_raw & (1 << bit_pos);
		break;
	case MODULE_PERIPH:
		bit_pos = 15;
		if (PWR_STATE_OFF == state) {
			bit_pos = bit_pos - 1;
		}
		ret = val_int_raw & (1 << bit_pos);
		break;
	default:
		ret = 0;
	}

	return (ret ? 1 : 0);

}

void pmu_init(void);

#endif /* PMU_H */
