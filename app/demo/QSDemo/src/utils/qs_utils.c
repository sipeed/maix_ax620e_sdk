/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "qs_utils.h"
#include "qs_log.h"
#include <stdlib.h>
#include "ax_sys_api.h"

AX_VOID MSSleep(AX_U32 nSleep) {
    usleep(nSleep * 1000);
}

/* check mount point */
AX_BOOL QS_CheckMounted(const char* mntpt, AX_S32 nTimes) {

    AX_BOOL bRet = AX_FALSE;
    AX_S32 nTimesTmp = nTimes;

    FILE *fp;
    AX_CHAR buf[256];
    memset(buf, 0, sizeof(buf));
    AX_CHAR command[256];
    memset(command, 0, sizeof(command));

    snprintf(command, sizeof(command), "cat /proc/mounts | grep '%s '", mntpt);
    do {
        fp = popen(command, "r");
        if (fp == NULL) {
            /* excute command failed */
            ALOGE("popen failed: %s.", command);
            break;
        }

        if (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
            bRet = AX_TRUE;
        }
        pclose(fp);
        if (bRet) {
            break;
        }
        nTimesTmp --;
        MSSleep(40);
    } while (nTimesTmp > 0);

    ALOGI("retry cat mounts times: %d.", nTimes - nTimesTmp);
    return bRet;
}

AX_S32 QS_GetSingleSnsPipeId(AX_S32 nDefault) {
    AX_CHAR szResult[64] = {0};
    AX_S32 nRet = 0;
    AX_S32 nValue = 0;
    nRet = QS_RunCmd("ax_env.sh print qs_single_sns_pipe_id  | awk -F '=' '{print $2}'", szResult, 64);
    if (nRet == 0 && strlen(szResult) > 0) {
        nValue = atoi(szResult);
        /* rx0:0 rx1:1 */
        if (nValue == 0 || nValue == 1) {
            return nValue;
        } else {
            ALOGI("invalid single_sns_pipe_id: %s, use default: %d", szResult, nDefault);
            return nDefault;
        }
    } else {
        return nDefault;
    }
}

AX_S32 QS_RunCmd(AX_CHAR* cmd, AX_CHAR *result, AX_S32 size) {
    if (!cmd) {
        return -1;
    }

    FILE * fp = NULL;

    fp = popen(cmd, "r");

    if(fp == NULL){
        return -1;
    }
    if (result && size > 0) {
        fread(result, 1, size - 1, fp);
    }
    pclose(fp);
    return 0;
}

AX_S32  QS_GetSc850sl2MFlag(AX_S32 nDefault) {
    AX_CHAR szResult[64] = {0};
    AX_S32 nRet = 0;
    AX_S32 nValue = 0;
    nRet = QS_RunCmd("ax_env.sh print qs_sc850sl2m | awk -F '=' '{print $2}'", szResult, 64);
    if (nRet == 0 && strlen(szResult) > 0) {
        nValue = atoi(szResult);
        if (nValue == 0 || nValue == 1) {
            return nValue;
        } else {
            ALOGI("invalid sc850sl2m flag: %s, use default: %d", szResult, nDefault);
            return nDefault;
        }
    } else {
        return nDefault;
    }
}

AX_S32  QS_SetSnsSwitch(AX_S32 nSnsId) {
    AX_CHAR szCmd[256] = {0};
    sprintf(szCmd, "ax_env.sh set qs_sns_switch %d", nSnsId);
    return QS_RunCmd(szCmd, NULL, 0);
}

AX_S32  QS_GetSnsSwitch(AX_S32 nDefault) {
    AX_CHAR szResult[64] = {0};
    AX_S32 nRet = 0;
    AX_S32 nValue = 0;
    nRet = QS_RunCmd("ax_env.sh print qs_sns_switch | awk -F '=' '{print $2}'", szResult, 64);
    if (nRet == 0 && strlen(szResult) > 0) {
        nValue = atoi(szResult);
        if (nValue == 1 || nValue == 2) {
            return nValue;
        } else {
            ALOGI("invalid qs_sns_switch value: %s, use default: %d", szResult, nDefault);
            return nDefault;
        }
    } else {
        return nDefault;
    }
}

const AX_CHAR* GetFlagFilePath(E_FLAG_FILE eFlag) {
    switch(eFlag) {
    case E_FLAG_FILE_TIMES:
    case E_FLAG_FILE_REBOOT:
    case E_FLAG_FILE_AOV:
        return "/customer";
    default:
        return "";
    }
}

const AX_CHAR* GetFlagFile(E_FLAG_FILE eFlag) {
    switch(eFlag) {
    case E_FLAG_FILE_TIMES:
        return "/customer/qs_times";
    case E_FLAG_FILE_REBOOT:
        return "/customer/reboot";
    case E_FLAG_FILE_AOV:
        return "/customer/aov";
    default:
        return "";
    }
}