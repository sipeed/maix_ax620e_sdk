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

/* types of variables typedef */
typedef unsigned long long int  AX_U64;
typedef unsigned int            AX_U32;
typedef unsigned short          AX_U16;
typedef unsigned char           AX_U8;
typedef long long int           AX_S64;
typedef int                     AX_S32;
typedef short                   AX_S16;
typedef char                    AX_S8;
typedef long                    AX_LONG;
typedef unsigned long           AX_ULONG;
typedef unsigned long           AX_ADDR;
typedef float                   AX_F32;
typedef double                  AX_F64;
typedef void                    AX_VOID;

typedef enum {
    AX_FALSE = 0,
    AX_TRUE  = 1,
} AX_BOOL;


#endif //_AX_BASE_TYPE_H_
