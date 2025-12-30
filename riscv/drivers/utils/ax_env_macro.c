/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_common.h"
#include "board.h"
#include "soc.h"

void ax_isp_tmp_delay(void)
{
#ifdef AX620E_EMMC
    ax_mdelay(20);
#endif
}

uint32_t ax_env_heap_end(void)
{
    return HEAP_RAM_END;
}

uint32_t ax_env_riscv_ddr_base(void)
{
    return RISCV_DDR_BASE;
}

uint32_t ax_env_riscv_mem_len_mb(void)
{
    return RISCV_MEM_LEN_MB;
}

uint32_t ax_env_isp_reserved_base(void)
{
    return ISP_RESERVED_DDR_BASE;
}

uint32_t ax_env_isp_reserved_end(void)
{
    return ISP_RESERVED_DDR_END;
}
