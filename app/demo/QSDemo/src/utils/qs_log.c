/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "qs_log.h"
#include <pthread.h>
#include <stdarg.h>

static AX_CHAR g_szLogPath[] = "/tmp/qsdemo.log";
static FILE *g_pLogFile = NULL;
pthread_mutex_t g_mtxLog = PTHREAD_MUTEX_INITIALIZER;

AX_BOOL LogOpen() {
    return AX_TRUE;
    // pthread_mutex_lock(&g_mtxLog);
    // if (!g_pLogFile) {
    //     g_pLogFile = fopen(g_szLogPath, "w");
    //     if (g_pLogFile) {
    //         pthread_mutex_unlock(&g_mtxLog);
    //         return AX_TRUE;
    //     }
    // }
    // pthread_mutex_unlock(&g_mtxLog);
    // printf("create log file %s failed, error=%d", g_szLogPath, errno);
    // return AX_FALSE;
}

AX_VOID LogToFile(const AX_CHAR *pFmt, ...) {
    pthread_mutex_lock(&g_mtxLog);
    if (!g_pLogFile) {
        g_pLogFile = fopen(g_szLogPath, "w");
        if (!g_pLogFile) {
            ALOGE("create log file %s failed, error=%d", g_szLogPath, errno);
            pthread_mutex_unlock(&g_mtxLog);
            return;
        }
    }

    AX_CHAR szBuf[1024] = {0};
    va_list args;
    AX_S32 nLen = 0;

    va_start(args, pFmt);
    nLen = vsnprintf(szBuf, sizeof(szBuf), pFmt, args);
    va_end(args);

    fwrite(szBuf, 1, nLen, g_pLogFile);
    fwrite("\n", 1, 1, g_pLogFile);
    fflush(g_pLogFile);

    ALOGW("%s", szBuf);
    pthread_mutex_unlock(&g_mtxLog);
}

AX_VOID LogClose() {
    pthread_mutex_lock(&g_mtxLog);
    if (g_pLogFile) {
        fclose(g_pLogFile);
        g_pLogFile = NULL;
    }
    pthread_mutex_unlock(&g_mtxLog);
}