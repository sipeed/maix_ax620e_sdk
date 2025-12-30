/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_UTILS_H_
#define _AX_OPAL_UTILS_H_

#include <stdlib.h>
#include "ax_global_type.h"
#include "ax_sys_api.h"

#define AX_OPAL_MALLOC(size) \
    ({  \
        AX_VOID *_MALLOC_PTR_ = malloc(size);   \
        _MALLOC_PTR_;   \
    })

#define AX_OPAL_FREE(ptr)    \
    ({  \
        if (ptr != NULL) {  \
            free(ptr);  \
            ptr = NULL; \
        }   \
    })

#ifndef AX_SWAP
//#define AX_SWAP(a, b) do { (a) = (a) ^ (b); (b) = (a) ^ (b); (a) = (a) ^ (b); } while(0)
#define AX_SWAP(a, b) do { typeof(a) t; t = (a); (a) = (b); (b) = t; } while(0)
#endif

#ifndef AX_MAX
#define AX_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef AX_MIN
#define AX_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#endif

#ifndef ALIGN_COMM_UP
#define ALIGN_COMM_UP(x, align) ((((x) + ((align)-1)) / (align)) * (align))
#endif

#ifndef ALIGN_COMM_DOWN
#define ALIGN_COMM_DOWN(x, align) (((x) / (align)) * (align))
#endif

#ifndef ADAPTER_RANGE
#define ADAPTER_RANGE(v, min, max) ((v) < (min)) ? (min) : ((v) > (max)) ? (max) : (v)
#endif

#ifndef AX_BIT_CHECK
#define AX_BIT_CHECK(v, b) (((AX_U32)(v) & (1 << (b))))
#endif

#ifndef AX_BIT_SET
#define AX_BIT_SET(v, b) ((v) |= (1 << (b)))
#endif

#ifndef AX_BIT_CLEAR
#define AX_BIT_CLEAR(v, b) ((v) &= ~(1 << (b)))
#endif

#ifndef AX_ABS
#define AX_ABS(a) (((a) < 0) ? -(a) : (a))
#endif

#ifndef AX_MASK
#define AX_MASK(width) ((1 << (width)) - 1) /* 2^w - 1 */
#endif

#ifndef IS_AX620Q
#define IS_AX620Q  (AX_SYS_GetChipType() != AX630C_CHIP)
#endif

#ifndef IS_SNS_LINEAR_MODE
#define IS_SNS_LINEAR_MODE(eSensorMode) (((eSensorMode == AX_SNS_LINEAR_MODE) || (eSensorMode == AX_SNS_LINEAR_ONLY_MODE)) ? AX_TRUE : AX_FALSE)
#endif

#ifndef IS_SNS_HDR_MODE
#define IS_SNS_HDR_MODE(eSensorMode) (((eSensorMode >= AX_SNS_HDR_2X_MODE) && (eSensorMode <= AX_SNS_HDR_4X_MODE)) ? AX_TRUE : AX_FALSE)
#endif

AX_VOID ax_opal_us_sleep(AX_U32 microseconds);
AX_VOID ax_opal_ms_sleep(AX_U32 milliseconds);
AX_U64  ax_opal_get_timestamp();
AX_U64  ax_opal_get_clocktime();
AX_U64  ax_opal_get_nanotime();

#endif // _AX_OPAL_UTILS_H_
