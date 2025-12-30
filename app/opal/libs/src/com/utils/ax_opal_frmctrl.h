/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPAL_FRMCTRL_H_
#define _AX_OPAL_FRMCTRL_H_

#include "ax_global_type.h"

typedef AX_VOID*  AX_FRMCTRL_HANDLE;

AX_S32  AX_OPAL_FrmCtrlCreate(AX_FRMCTRL_HANDLE *pHandle, AX_U32 nSrcFrmRate, AX_U32 nDstFrmRate);
AX_VOID AX_OPAL_FrmCtrlDestroy(AX_FRMCTRL_HANDLE pHandle);
AX_BOOL AX_OPAL_FrmCtrlReset(AX_FRMCTRL_HANDLE pHandle, AX_U32 nSrcFrmRate, AX_U32 nDstFrmRate);
AX_BOOL AX_OPAL_FrmCtrlFilter(AX_FRMCTRL_HANDLE pHandle, AX_BOOL bTry);


#endif // _AX_OPAL_FRMCTRL_H_