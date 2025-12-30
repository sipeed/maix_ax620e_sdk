/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <stdint.h>
#include "riscv_regs.h"
#include "riscv_sw_int.h"
#include "cmn.h"

static uint32_t sw_base_addr[sw_int_group_max] = {
	SW_INT0_BASE,
	SW_INT1_BASE,
	SW_INT2_BASE,
	SW_INT3_BASE
};

void riscv_sw_int_trigger(sw_int_group_e group, sw_int_channel_e channel)
{
	uint32_t trigger_reg_addr = sw_base_addr[group] + SW_INT_TRIGGER_OFFSET;
	uint32_t trigger_value = 1 << channel;
	writel(trigger_value, trigger_reg_addr);
}

