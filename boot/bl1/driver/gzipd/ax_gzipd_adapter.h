/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_GZIPD_ADAPTER_H_
#define _AX_GZIPD_ADAPTER_H_

#include "cmn.h"
#include "../usb/printf.h"
#include "../include/timer.h"

// #define AX_GZIP_DEBUG_LOG_EN
// #define AX_GZIPD_BYPASS_EN
// #define AX_GZIPD_CRC_ENABLE

#define FALSE   0
#define TRUE    1

#define __raw_readl(x) readl((unsigned long)x)
#define __raw_writel(v, x) writel(v, (unsigned long)x)
#define memcpy(to, from, n) ax_memcpy(to, from, n)

#ifdef AX_GZIP_DEBUG_LOG_EN
#define AX_GZIP_LOG_DBG(fmt, arg...) info("[GZIP][D]: [%s : %d] " fmt "\r\n", __func__, __LINE__, ##arg)
#else
#define AX_GZIP_LOG_DBG(fmt, arg...)
#endif
#define AX_GZIP_LOG_ERR(fmt, arg...) info("[GZIP][E]: [%s : %d] " fmt "\r\n", __func__, __LINE__, ##arg)

#endif