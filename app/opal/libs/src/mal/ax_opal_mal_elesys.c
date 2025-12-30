/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_opal_mal_elesys.h"

#include "ax_opal_log.h"
#include "ax_opal_utils.h"

#define LOG_TAG ("ELESYS")

// interface vtable
AX_OPAL_MAL_ELE_Interface sys_interface = {
    .event_proc = AX_OPAL_MAL_ELESYS_Process,
};

AX_OPAL_MAL_ELE_HANDLE AX_OPAL_MAL_ELESYS_Create(AX_OPAL_MAL_SUBPPL_HANDLE parent, AX_OPAL_ELEMENT_ATTR_T *pEleAttr) {

    AX_OPAL_MAL_ELE_HANDLE handle = AX_OPAL_MALLOC(sizeof(AX_OPAL_MAL_ELESYS_T));
    if (handle == AX_NULL) {
        LOG_M_E(LOG_TAG, "malloc failed.");
        return AX_NULL;
    }

    AX_OPAL_MAL_ELESYS_T* ele = (AX_OPAL_MAL_ELESYS_T*)handle;
    memset(ele, 0x0, sizeof(AX_OPAL_MAL_ELESYS_T));
    ele->stBase.nId = pEleAttr->nId;
    ele->stBase.pParent = parent;
    ele->stBase.vTable = &sys_interface;

    return handle;
}

// AX_S32 AX_OPAL_MAL_ELESYS_Destroy(AX_OPAL_MAL_ELE_HANDLE self) {
//     AX_S32 nRet = AX_SUCCESS;

//     if (self == AX_NULL) {
//         return AX_ERR_OPAL_NULL_PTR;
//     }
//     AX_OPAL_FREE(self);

//     return nRet;
// }

AX_S32 AX_OPAL_MAL_ELESYS_Process(AX_OPAL_MAL_ELE_HANDLE self, AX_OPAL_MAL_PROCESS_DATA_T *pPorcessData) {
    AX_S32 nRet = AX_SUCCESS;


    return nRet;
}
