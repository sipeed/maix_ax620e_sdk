
/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#ifndef _AX_OPAL_MAL_PPL_PARSER_H_
#define _AX_OPAL_MAL_PPL_PARSER_H_

#include "ax_opal_mal_def.h"
#include "ax_opal_hal_sys.h"

AX_S32 AX_OPAL_MAL_PPL_Parse(const AX_CHAR* pFileName, AX_OPAL_PPL_ATTR_T* pPplCfg);

AX_S32 AX_OPAL_MAL_POOL_Parse(const AX_CHAR* pFileName, AX_OPAL_POOL_ATTR_T* pPoolCfg);

#endif // _AX_OPAL_MAL_PPL_PARSER_H_
