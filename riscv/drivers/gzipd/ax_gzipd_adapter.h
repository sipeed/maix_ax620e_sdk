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

#include "ax_common.h"

#define FALSE   0
#define TRUE    1

#ifdef AX_GZIP_DEBUG_LOG_EN
#define AX_GZIP_LOG_DBG(fmt, arg...) rt_kprintf("[GZIP][D]: [%s : %d] " fmt "\n", __func__, __LINE__, ##arg)
#else
#define AX_GZIP_LOG_DBG(fmt, arg...)
#endif
#define AX_GZIP_LOG_ERR(fmt, arg...) rt_kprintf("[GZIP][E]: [%s : %d] " fmt "\n", __func__, __LINE__, ##arg)

#endif
