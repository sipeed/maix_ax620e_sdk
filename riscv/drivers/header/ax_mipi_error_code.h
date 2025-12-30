/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_MIPI_ERROR_CODE_H_
#define _AX_MIPI_ERROR_CODE_H_

#include "ax_global_type.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
|----------------------------------------------------------------|
||   FIXED   |   MOD_ID    | SUB_MODULE_ID |   ERR_ID            |
|----------------------------------------------------------------|
|<--8bits----><----8bits---><-----8bits---><------8bits------->|
******************************************************************************/

/* MIPI Error Code Base: 0x800exxxx */
typedef enum {
    AX_ID_MIPI_ERR     = 0x01,
    AX_ID_MIPI_BUTT,
} AX_MIPI_SUB_ID_E;

typedef enum {
    AX_ERR_CODE_MIPI_FAILED                   = 0x80,   /* MIPI Failed */
} AX_VIN_ERR_CODE_E;

/* Success */
#define AX_MIPI_SUCCESS                      (0)

/* Common Err Code */
#define AX_ERR_MIPI_NULL_PTR                 AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_NULL_PTR)
#define AX_ERR_MIPI_INVALID_DEVID            AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_INVALID_DEVID)
#define AX_ERR_MIPI_ILLEGAL_PARAM            AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_ILLEGAL_PARAM)
#define AX_ERR_MIPI_NOT_SUPPORT              AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_NOT_SUPPORT)
#define AX_ERR_MIPI_NOMEM                    AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_NOMEM)
#define AX_ERR_MIPI_TIMEOUT                  AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_TIMED_OUT)
#define AX_ERR_MIPI_NOT_INIT                 AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_NOT_INIT)
#define AX_ERR_MIPI_ATTR_NOT_CFG             AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_NOT_CONFIG)
#define AX_ERR_MIPI_INVALID_ADDR             AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_BAD_ADDR)

/* Private Err Code */
#define AX_ERR_MIPI_FAILED                   AX_DEF_ERR(AX_ID_MIPI, AX_ID_MIPI_ERR, AX_ERR_CODE_MIPI_FAILED)

#define AX_MIPI_ERR_CODE(MODULE, ERR_CODE)   AX_DEF_ERR(AX_ID_MIPI, MODULE, ERR_CODE)

#ifdef __cplusplus
}
#endif

#endif //_AX_MIPI_ERROR_CODE_H_

