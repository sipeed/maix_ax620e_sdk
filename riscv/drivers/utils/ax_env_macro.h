/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_ENV_MACRO_H__
#define __AX_ENV_MACRO_H__

void ax_isp_tmp_delay(void);
uint32_t ax_env_heap_end(void);
uint32_t ax_env_riscv_ddr_base(void);
uint32_t ax_env_riscv_mem_len_mb(void);
uint32_t ax_env_isp_reserved_base(void);
uint32_t ax_env_isp_reserved_end(void);

#endif //__AX_ENV_MACRO_H__
