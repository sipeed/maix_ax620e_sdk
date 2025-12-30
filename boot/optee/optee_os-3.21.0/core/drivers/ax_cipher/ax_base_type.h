/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_BASE_TYPE_H_
#define _AX_BASE_TYPE_H_
#include <stdbool.h>
#include <trace.h>
#include <io.h>

#define ax_cipher_err(...)      EMSG(__VA_ARGS__)
#define ax_cipher_info(...)	(void)0
#define ax_cipher_dbg(...)	(void)0

#define readl(addr) io_read32((vaddr_t)addr)
#define writel(b, addr) io_write32((vaddr_t)addr, b)

#endif //_AX_BASE_TYPE_H_
