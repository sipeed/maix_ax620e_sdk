/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "web_server.h"
#include "ax_opal_api.h"
#include "ax_opal_type.h"
#include "ax_log.h"
#include "ax_thread.h"
#include "ax_timer.h"
#include "ax_utils.h"
#include "ax_option.h"
#include "ax_map.h"
#include "cJSON.h"
#include "AXRingFifo.h"
#include "config.h"
#include "GlobalDef.h"

#include "appweb.h"
#include "http.h"
#include <sys/prctl.h>
#include <pthread.h>

#define WEB "WEB SERVER"

#define RESPONSE_STATUS_OK "200"
#define RESPONSE_STATUS_AUTH_FAIL "401"
#define RESPONSE_STATUS_INVALID_REQ "400"
#define RESPONSE_STATUS_FAILURE "406"
#define RESPONSE_STATUS_OK_CODE 200
#define RESPONSE_STATUS_AUTH_FAIL_CODE 401
#define RESPONSE_STATUS_INVALID_REQ_CODE 400
#define RESPONSE_STATUS_FAILURE_CODE 406
#define PARAM_KEY_PREVIEW_SNS_SRC "src_id"
#define PARAM_KEY_PREVIEW_CHANNEL "stream"
#define PARAM_KEY_APP_NAME "appName"
#define PARAM_KEY_APP_VERSION "appVersion"
#define PARAM_KEY_SDK_VERSION "sdkVersion"
#define PARAM_KEY_SETTING_CAPABILITY "capInfo"
#define PARAM_KEY_SNS_MODE "snsMode"

#define MAX_WS_VEIDO_CONN_NUM       (MAX_PREV_SNS_NUM * MAX_PREV_SNS_CHN_NUM)
#define MAX_WS_CONN_NUM             (MAX_WS_VEIDO_CONN_NUM + 5)
#define MAX_EVENTS_CHN_SIZE         (256)
#define PTS_MAGIC                   (0x54495841)  // "AXIT" by default
#define MAX_WSG_COUNT               (128)
#define MAX_TOKEN_STR_LEN           (128)
#define MAX_TOKEN_CONN_SIZE         (256)
#define WEB_INVLID_ID               0xFF

#define WEB_USER                     "admin"
#define WEB_PWD                      "admin"


typedef struct _WS_MSG_T{
    HttpConn* conn;
    AX_RINGFIFO_ELEMENT_T  packet;
    AX_U8 nUniChn;
    AX_BOOL   valid;
} WS_MSG_T;

typedef struct _PTS_HEADER_T {
    AX_U32 nMagic;
    AX_U32 nDatalen;
    AX_U64 nPts;
} PTS_HEADER_T;

typedef struct _WS_CHN_TOKEN_T {
    AX_CHAR szToken[MAX_TOKEN_STR_LEN];
    AX_U8   nPrevUniChn[MAX_PREV_SNS_NUM];
    AX_U8   bValid;
} WS_CHN_TOKEN_T;

typedef struct _WS_USER_TOKEN_T {
    AX_CHAR szUser[MAX_TOKEN_STR_LEN];
    AX_CHAR szToken[MAX_TOKEN_STR_LEN];
    AX_U8   bValid;
} WS_USER_TOKEN_T;

typedef struct _WS_CHN_ITEM_T {
    AX_RINGFIFO_HANDLE hRingFifo;
    AX_U8 nSnsId;
    AX_U8 nUniChn;
    AX_U8 nPrevChn;
    AX_U8 nSrcChn;
    AX_U8 bValid;
    AX_U8 bConnected;
    AX_U16 nMaxWidth;
    AX_U16 nMaxHeight;
    AX_U32 nOutBytes;
    AX_U64 nStartTick;
    WS_MSG_T arrWsMsg[MAX_WSG_COUNT];
    AX_S32 nWsMsgUsed;
    pthread_mutex_t mtxWsMsg;
} WS_CHN_ITEM_T;

typedef struct _WS_INFO_T {
    AX_BOOL bServerStarted;
    AX_BOOL bAudioCaptureAvailable;
    AX_BOOL bAudioPlayAvailable;
    AX_BOOL bStatusCheckStarted;
    OPAL_THREAD_T* pAppwebThread;
    OPAL_THREAD_T* pSendDataThread;
    OPAL_THREAD_T* pStatucCheckThread;

    WS_CHN_ITEM_T stChnVideo[MAX_PREV_SNS_NUM][MAX_PREV_SNS_CHN_NUM];
    WS_CHN_ITEM_T stChnAudio;
    WS_CHN_ITEM_T stChnImage;
    WS_CHN_ITEM_T stChnEvent;

    WS_CHN_ITEM_T stChnSnapshot; //dummy
    WS_CHN_ITEM_T stChnTalk;     //dummy

    AX_U8         nSnsPrevChnCount[MAX_PREV_SNS_NUM];
    AX_U8         nSnsCount;
} WS_INFO_T;

static WS_CHN_ITEM_T* g_pChnList[MAX_WS_CONN_NUM] = {0};

static WS_INFO_T g_stWsInfo = {0};

static pthread_mutex_t  g_mtxConnStatus  = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  g_mtxStatusCheck = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   g_cvStatusCheck  = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t  g_mtxVencStat = PTHREAD_MUTEX_INITIALIZER;

static AX_S8 g_nSnapshotChannel = (MAX_WS_VEIDO_CONN_NUM + 0);
static AX_S8 g_nEventChannel    = (MAX_WS_VEIDO_CONN_NUM + 1);
static AX_S8 g_nPushImgChannel  = (MAX_WS_VEIDO_CONN_NUM + 2);
static AX_S8 g_nAudioChannel    = (MAX_WS_VEIDO_CONN_NUM + 3);
static AX_S8 g_nTalkChannel     = (MAX_WS_VEIDO_CONN_NUM + 4);


static AX_BOOL  g_web_mem_limit_notified = AX_FALSE;
static MprList* g_pClients = NULL;

static ax_map_handle    g_mapUser2Token = NULL;

extern AX_OPAL_ATTR_T g_stOpalAttr;
extern AX_U8 g_opal_nSnsNum;

static WS_MSG_T* GetWsMsgData(AX_U8 nUniChn) {
    if (nUniChn >= MAX_WS_CONN_NUM) {
        return NULL;
    }
    pthread_mutex_lock(&g_pChnList[nUniChn]->mtxWsMsg);
    for (AX_U32 i = 0; i < MAX_WSG_COUNT; i++) {
        if (g_pChnList[nUniChn]->arrWsMsg[i].valid == 0) {
            g_pChnList[nUniChn]->arrWsMsg[i].valid = 1;
            g_pChnList[nUniChn]->nWsMsgUsed++;
            pthread_mutex_unlock(&g_pChnList[nUniChn]->mtxWsMsg);
            return &g_pChnList[nUniChn]->arrWsMsg[i];
        }
    }
    pthread_mutex_unlock(&g_pChnList[nUniChn]->mtxWsMsg);
    return NULL;
}

static AX_VOID ReleaseWsMsgData(WS_MSG_T* pData) {
    if (pData) {
        AX_U8 nUniChn = pData->nUniChn;
        if (nUniChn < MAX_WS_CONN_NUM) {
            pthread_mutex_lock(&g_pChnList[nUniChn]->mtxWsMsg);
            pData->valid = 0;
            g_pChnList[nUniChn]->nWsMsgUsed--;
            pthread_mutex_unlock(&g_pChnList[nUniChn]->mtxWsMsg);
        }
    }
}

// strong
void* MprVmalloc(size_t size, int mode) {
    void *ptr;

#if 1
    ptr = (void*)AX_MALLOC(size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
#else
    if ((ptr = mmap(0, size, mode, MAP_PRIVATE | MAP_ANON, -1, 0)) == (void*) -1) {
        return 0;
    }
#endif

    return ptr;
}

void MprVmfree(void *ptr, size_t size) {
#if 1
    AX_FREE(ptr);
#else
    munmap(ptr, size);
#endif
}

static AX_VOID WebMprYield() {
#if 0
    if (mprNeedYield()) {
        mprYield(MPR_YIELD_DEFAULT);
    }
#endif
}

static AX_VOID UpdateVencStat(AX_U8 nUniChn, AX_U32 nSize) {
    if (nUniChn < MAX_WS_CONN_NUM) {
        pthread_mutex_lock(&g_mtxVencStat);
        g_pChnList[nUniChn]->nOutBytes += nSize;
        if (g_pChnList[nUniChn]->nStartTick == 0) {
            g_pChnList[nUniChn]->nStartTick = OPAL_GetTickCount();
        }
        pthread_mutex_unlock(&g_mtxVencStat);
    }
}

static AX_F64 GetVencStatBitrate(AX_U8 nUniChn) {
    AX_F64 fBitrate = 0;
    if (nUniChn < MAX_WS_CONN_NUM) {
        pthread_mutex_lock(&g_mtxVencStat);
        AX_U64 nEndTick = OPAL_GetTickCount();
        AX_U64 nGap = nEndTick - g_pChnList[nUniChn]->nStartTick;
        fBitrate = g_pChnList[nUniChn]->nOutBytes / (AX_F64)nGap * 8;
        g_pChnList[nUniChn]->nOutBytes = 0;
        g_pChnList[nUniChn]->nStartTick = nEndTick;
        pthread_mutex_unlock(&g_mtxVencStat);
    }
    return fBitrate;
}

static AX_S32 YieldProcessWebOpr(HttpConn* conn, WEB_REQUEST_TYPE_E requestType, AX_VOID** pResult, AX_BOOL bNeedYield) {
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    // if(bNeedYield) {
    //     mprYield(MPR_YIELD_STICKY);
    // }

    MprJson* jsonParam = httpGetParams(conn);


    if (!ProcWebRequest(requestType, jsonParam, pResult)){
        nHttpStatusCode = RESPONSE_STATUS_FAILURE_CODE;
    }

    // if(bNeedYield) {
    //     mprResetYield();
    // }

    return nHttpStatusCode;
}

static AX_VOID* MprListGetNextItem(MprList* lp, AX_S32* next) {
    AX_VOID* item = NULL;
    int index = 0;

    if (lp == 0 || lp->length == 0) {
        return NULL;
    }
    index = *next;
    if (index < lp->length) {
        item = lp->items[index];
        *next = ++index;
        return item;
    }
    return NULL;
}

static AX_VOID WebServerMemNotifier(int cause, int policy, size_t size, size_t total) {
    switch (cause) {
    case MPR_MEM_LIMIT:
        {
            HttpConn* client = NULL;
            MprTicks now = mprGetTicks();
            MprTicks requestTimeout = 0;
            const MprTicks minRequestTimeout = 300000; // 5mins

            mprLock(g_pClients->mutex);
            // modify RequestTimeout
            for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
                if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
                    continue;
                }

                if (client->limits->requestTimeout > minRequestTimeout) {
                    if (now > client->started) {
                        requestTimeout = now - client->started;
                    }

                    if (requestTimeout < minRequestTimeout) {
                        requestTimeout = minRequestTimeout;
                    }

                    httpSetTimeout(client, requestTimeout, -1);

                    g_web_mem_limit_notified = AX_TRUE;
                }
            }
            mprUnlock(g_pClients->mutex);
        }
        break;

    default:
        break;
    }
}


static AX_U8 GetSnsIDFromWS(HttpConn* conn) {
    if (!conn) {
        return WEB_INVLID_ID;
    }
    AX_U32 nWSData = (size_t)conn->staticData;
    return (AX_U8)(nWSData & 0xFF);
}

static AX_U8 GetUniChnFromWS(HttpConn* conn) {
    if (!conn) {
        return WEB_INVLID_ID;
    }
    AX_U32 nWSData = (size_t)conn->staticData;
    return (AX_U8)(nWSData >> 8);
}


/* http event callback */

static AX_VOID SendHttpData(WS_MSG_T* msg) {
    HttpConn* stream = msg->conn;
    AX_RINGFIFO_ELEMENT_T stData = msg->packet;
    ReleaseWsMsgData(msg);

    // if ((mprLookupItem(g_pClients, stream) < 0)) {
    //     AX_RingFifo_Free(AX_RingFifo_GetHandle(&stData), &stData, AX_FALSE);
    //     return;
    // }
    if (!WS_IsRunning()) {
        AX_RingFifo_Free(AX_RingFifo_GetHandle(&stData), &stData, AX_FALSE);
        return;
    }

    AX_U8* pBuf = stData.data[0].buf;
    AX_S32 nSize = (AX_S32)(stData.data[0].len);
    AX_U8* pBuf2 = stData.data[1].buf;
    AX_S32 nSize2 = (AX_S32)(stData.data[1].len);

    do {
        if (stream == NULL || stream->connError || stream->timeout != 0 || !pBuf || nSize == 0) {
            break;
        }
        if (WS_STATE_OPEN != httpGetWebSocketState(stream)) {
            break;
        }
        // AX_U8 nSnsId = GetSnsIDFromWS(stream);
        // AX_U8 nUniChn = GetUniChnFromWS(stream);
        // LOG_M_C(WEB, "nSnsId=%d, nUniChn=%d", nSnsId, nUniChn);
        ssize nRet = 0;
        if (nSize2 > 0) {
            nRet = httpSendBlock(stream, WS_MSG_BINARY, (cchar*)pBuf, nSize, HTTP_BLOCK | HTTP_MORE);
        } else {
            nRet = httpSendBlock(stream, WS_MSG_BINARY, (cchar*)pBuf, nSize, HTTP_BLOCK);
        }
        if (nRet == nSize) {
            if (nSize2 > 0 && pBuf2) {
                nRet = httpSendBlock(stream, WS_MSG_BINARY, (cchar*)pBuf2, nSize2, HTTP_BLOCK);
                if (nRet == nSize2) {
                    break;
                }
            } else {
                break;
            }
        }

        switch (nRet) {
            case MPR_ERR_TIMEOUT:
                LOG_M_E(WEB, "httpSendBlock() return ERR_TIMEOUT.");
                break;
            case MPR_ERR_MEMORY:
                LOG_M_E(WEB, "httpSendBlock() return ERR_MEMORY.");
                break;
            case MPR_ERR_BAD_STATE:
                LOG_M_E(WEB, "httpSendBlock() return MPR_ERR_BAD_STATE.");
                break;
            case MPR_ERR_BAD_ARGS:
                LOG_M_E(WEB, "httpSendBlock() return MPR_ERR_BAD_ARGS.");
                break;
            case MPR_ERR_WONT_FIT:
                LOG_M_E(WEB, "httpSendBlock() return MPR_ERR_WONT_FIT.");
                break;
            default:
                LOG_M_E(WEB, "httpSendBlock failed, nRet=%d", (AX_S32)nRet);
                break;
        }
    } while (0);
    AX_RingFifo_Free(AX_RingFifo_GetHandle(&stData), &stData, AX_FALSE);

}

static size_t GenKeyData(AX_U8 nSnsId, AX_U8 nChnID) {
    return (size_t)(nSnsId | (size_t)nChnID << 8);
}

static AX_U8 GetUniChn(AX_U8 nSnsId, AX_U8 nPrevChn) {
    for(AX_S32 i = 0; i < MAX_PREV_SNS_CHN_NUM; i++) {
        if (g_stWsInfo.stChnVideo[nSnsId][i].nPrevChn == nPrevChn) {
            return g_stWsInfo.stChnVideo[nSnsId][i].nUniChn;
        }
    }
    return WEB_INVLID_ID;
}

static AX_U8 GetUniChnBySrcChn(AX_U8 nSnsId, AX_U8 nSrcChn) {
    for(AX_S32 i = 0; i < MAX_PREV_SNS_CHN_NUM; i++) {
        if (g_stWsInfo.stChnVideo[nSnsId][i].nSrcChn == nSrcChn) {
            return g_stWsInfo.stChnVideo[nSnsId][i].nUniChn;
        }
    }
    return WEB_INVLID_ID;
}

AX_U8 GetSrcChn(AX_U8 nSnsId, AX_U8 nPrevChn) {
    for(AX_S32 i = 0; i < MAX_PREV_SNS_CHN_NUM; i++) {
        if (g_stWsInfo.stChnVideo[nSnsId][i].nPrevChn == nPrevChn) {
            return g_stWsInfo.stChnVideo[nSnsId][i].nSrcChn;
        }
    }
    return WEB_INVLID_ID;
}

AX_U8 GetVideoCount(AX_U8 nSnsId) {
    if (nSnsId < g_stWsInfo.nSnsCount) {
        return g_stWsInfo.nSnsPrevChnCount[nSnsId];
    }

    return 0;
}


static AX_BOOL CheckUser(HttpConn* conn, cchar* user, cchar* pwd) {
    if(strcmp(WEB_USER, user) == 0 && strcmp(WEB_PWD, pwd) == 0) {
        return AX_TRUE;
    }
    return AX_FALSE;
}

static cchar* GenToken(cchar* user, cchar* pwd) {
    uint64 nTickcount = mprGetHiResTicks();
    cchar *key = sfmt("%s_%lld", user, nTickcount);
    cchar *val = mprGetSHABase64(sfmt("token:%s-%s-%lld", user, pwd, nTickcount));
    ax_map_ss_put(g_mapUser2Token, key, val);
    return ax_map_ss_get(g_mapUser2Token, key)->val;
}

static cJSON * ConstructBaseResponse(AX_S32 nHttpStatusCode, cchar* pszToken) {
    cJSON* root = cJSON_CreateObject();
    cJSON* data = cJSON_CreateObject();
    cJSON* meta = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    cJSON_AddItemToObject(root, "meta", meta);
    if (pszToken) {
        cJSON_AddStringToObject(data, "token", pszToken);
    }
    cJSON_AddNumberToObject(meta, "status", nHttpStatusCode);
    return root;
}

static AX_BOOL httpWriteJson(HttpConn* conn, cJSON *json) {
    if (conn && conn->writeq && json) {
        AX_CHAR szBuf[4096] = {0};
        if(cJSON_PrintPreallocated(json, szBuf, 4096, 0)) {
            httpWrite(conn->writeq, szBuf);
            cJSON_Delete(json);
            return AX_TRUE;
        }
    }
    return AX_FALSE;
}

static cchar* GetTokenFromConn(HttpConn* conn, AX_BOOL bFromHeader) {
    if (!conn) {
        return NULL;
    }

    cchar* szToken = NULL;
    if (bFromHeader) {
        szToken = (httpGetHeader(conn, "authorization"));
    } else {
        szToken = (httpGetParam(conn, "token", NULL));
    }

    char* p = NULL;
    while ((p = schr(szToken, ' ')) != 0) {
        *p = '+';
    }

    return szToken;
}

static AX_U8 GetSnsIdFromConn(HttpConn* conn) {
    if (!conn) {
        return WEB_INVLID_ID;
    }

    cchar* szSnsId = (httpGetParam(conn, "sns_id", NULL));
    if (!szSnsId) {
        return WEB_INVLID_ID;
    }

    return atoi(szSnsId);
}

static AX_U8 GetChnIdFromConn(HttpConn* conn) {
    if (!conn) {
        return WEB_INVLID_ID;
    }

    cchar* szChnId = (httpGetParam(conn, "chn_id", NULL));
    if (!szChnId) {
        return WEB_INVLID_ID;
    }

    return atoi(szChnId);
}

static AX_BOOL IsAuthorized(HttpConn* conn, AX_BOOL bGetTokenFromHeader) {
    if (!WS_IsRunning()) {
        return AX_FALSE;
    }
    cchar* szToken = GetTokenFromConn(conn, bGetTokenFromHeader);
    if (0 == szToken || strlen(szToken) == 0) {
        return AX_FALSE;
    }

    ax_map_ssnd_t* node = NULL;
    for(node = (ax_map_ssnd_t*)ax_map_first(g_mapUser2Token); node; ) {
        if (strcmp(node->val, szToken) == 0) {
            return AX_TRUE;
        }
        node = ax_map_next(g_mapUser2Token, &node->node);
    }
    return AX_FALSE;
}

static AX_VOID ResponseUnauthorized(HttpConn* conn) {
    //LOG_M_I(WEB, "Unauthorized, try to login again.");
    cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_AUTH_FAIL_CODE, 0);
    httpSetContentType(conn, "application/json");
    httpWriteJson(conn, pResponseBody);
    httpSetStatus(conn, RESPONSE_STATUS_OK_CODE);
    httpFinalize(conn);
    WebMprYield();
}

static AX_VOID ResponseStatusCode(HttpConn* conn, AX_S32 nHttpStatusCode) {
    cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
    httpSetContentType(conn, "application/json");
    httpWriteJson(conn, pResponseBody);
    httpSetStatus(conn, nHttpStatusCode);
    httpFinalize(conn);
    WebMprYield();
}

static AX_VOID ResponseStatusCodeWithJson(HttpConn* conn, AX_S32 nHttpStatusCode, cJSON* pResponseBody) {
    httpSetContentType(conn, "application/json");
    httpWriteJson(conn, pResponseBody);
    httpSetStatus(conn, nHttpStatusCode);
    httpFinalize(conn);
    WebMprYield();
}

static AX_VOID ReadAndPlayAudioData(HttpConn* conn) {
    AX_U32 nChannel = GetUniChnFromWS(conn);
    if (nChannel != g_nTalkChannel) {
        return;
    }

    HttpPacket *packet = httpGetPacket(conn->readq);
    if ((packet != NULL) &&
        (packet->type == WS_MSG_BINARY)) {
        // FIXME: webapp only support PCM
        ssize nSize = httpGetPacketLength(packet);
        AX_OPAL_Audio_Play(AX_OPAL_AUDIO_CHN_0, PT_LPCM, (AX_U8*)httpGetPacketStart(packet), nSize);
    }
}
/*
static AX_VOID UpdateConnStatus(AX_VOID) {
    AX_U8 nUniChn = WEB_INVLID_ID;
    AX_U8 arrConnStatus[MAX_WS_CONN_NUM] = {AX_FALSE};
    HttpConn* client = NULL;

    mprLock(g_pClients->mutex);
    for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
            continue;
        }
        nUniChn = GetUniChnFromWS(client);
        if (nUniChn == WEB_INVLID_ID || nUniChn >= MAX_WS_CONN_NUM) {
            continue;
        }
        arrConnStatus[nUniChn] = AX_TRUE;
    }

    mprUnlock(g_pClients->mutex);

    pthread_mutex_lock(&g_mtxConnStatus);
    for (AX_U32 i = 0; i < MAX_WS_CONN_NUM; ++i) {
        g_pChnList[i]->bConnected = arrConnStatus[i];
    }
    pthread_mutex_unlock(&g_mtxConnStatus);
}
*/
static AX_VOID WebNotifier(HttpConn* conn, AX_S32 event, AX_S32 arg) {
    if ((event == HTTP_EVENT_APP_CLOSE) || (event == HTTP_EVENT_ERROR) || (event == HTTP_EVENT_DESTROY)) {
        AX_S32 nIndex = mprRemoveItem(g_pClients, conn);
        if (nIndex >= 0) {
            AX_U8 nSnsId = GetSnsIDFromWS(conn);
            AX_U8 nUniChn = GetUniChnFromWS(conn);
            if (nUniChn < MAX_WS_CONN_NUM) {
                g_pChnList[nUniChn]->bConnected = AX_FALSE;
                LOG_M_N(WEB,"sns=%d, uni=%d, is disconnected", nSnsId, nUniChn);
            }
            //UpdateConnStatus();
        }
    } else if (event == HTTP_EVENT_READABLE) {
        ReadAndPlayAudioData(conn);
    }
}

static AX_VOID LoginAction(HttpConn* conn) {
    if (!WS_IsRunning()) {
        LOG_M_E(WEB, "not started");
        return;
    }
    LOG_M_C(WEB, "...");
    cchar* strUser = httpGetParam(conn, "username", "unspecified");
    cchar* strPwd = httpGetParam(conn, "password", "unspecified");
    cchar* szToken = NULL;
    AX_S32 nStatus = 0;
    AX_BOOL bAuthRet = CheckUser(conn, strUser, strPwd);
    if (!bAuthRet) {
        nStatus = RESPONSE_STATUS_AUTH_FAIL_CODE;
        LOG_M_E(WEB, "check user failed");
    } else {
        nStatus = RESPONSE_STATUS_OK_CODE;
        szToken = GenToken(strUser, strPwd);
    }
    cJSON* pResponseBody = ConstructBaseResponse(nStatus, szToken);
    cJSON* data = cJSON_GetObjectItem(pResponseBody, "data");
    AX_U8 nSnsMode = (g_stWsInfo.nSnsCount > 1) ? 2 : 1;
    cJSON_AddNumberToObject(data, PARAM_KEY_SNS_MODE, nSnsMode);
    ResponseStatusCodeWithJson(conn, RESPONSE_STATUS_OK_CODE, pResponseBody);
}

static AX_VOID CapabilityAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
    cJSON_AddItemToObject(cJSON_GetObjectItem(pResponseBody, "data"), PARAM_KEY_SETTING_CAPABILITY, GetCapSettingJson());
    ResponseStatusCodeWithJson(conn, RESPONSE_STATUS_OK_CODE, pResponseBody);
}

static AX_VOID PreviewInfoAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
    cJSON_AddItemToObject(cJSON_GetObjectItem(pResponseBody, "data"), "info", GetPreviewInfoJson(&g_stWsInfo.nSnsPrevChnCount[0]));
    ResponseStatusCodeWithJson(conn, RESPONSE_STATUS_OK_CODE, pResponseBody);
}

static AX_VOID SaveWSConnection(HttpConn* conn, AX_U8 nSnsId, AX_U8 nUniChn) {
    conn->staticData = (void*)GenKeyData(nSnsId, nUniChn);
    mprAddItem(g_pClients, conn);
    httpSetConnNotifier(conn, WebNotifier);
    WebMprYield();
}

static AX_VOID WSPreviewAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        ResponseUnauthorized(conn);
        return;
    }
    AX_U8 nSnsId = GetSnsIdFromConn(conn);
    if (nSnsId >= MAX_PREV_SNS_NUM) {
        return;
    }
    AX_U8 nPreId = GetChnIdFromConn(conn);
    if (nPreId >= MAX_PREV_SNS_CHN_NUM) {
        return;
    }
    AX_U8 nUniChn = GetUniChn(nSnsId, nPreId);
    if (nUniChn != WEB_INVLID_ID && nUniChn < MAX_WS_CONN_NUM) {
        g_pChnList[nUniChn]->bConnected = AX_TRUE;
        SaveWSConnection(conn, nSnsId, nUniChn);
        LOG_M_N(WEB,"sns=%d, uni=%d, is connected", nSnsId, nUniChn);
    }
}

static AX_VOID WSPushImageAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        ResponseUnauthorized(conn);
        return;
    }
    SaveWSConnection(conn, 0, g_nPushImgChannel);
}

static AX_VOID WSSnapshotAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        ResponseUnauthorized(conn);
        return;
    }
    SaveWSConnection(conn, 0, g_nSnapshotChannel);
}

static AX_VOID SnapshotAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    LOG_M_C(WEB, "...");
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    static AX_BOOL bActionProcessing = AX_FALSE;
    if (!bActionProcessing) {
        bActionProcessing = AX_TRUE;
        nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_CAPTURE, (AX_VOID**)&conn, AX_TRUE);

        bActionProcessing = AX_FALSE;
    } else {
        // Response status: Not Acceptable
        nHttpStatusCode = RESPONSE_STATUS_FAILURE_CODE;
    }
    ResponseStatusCode(conn, nHttpStatusCode);
}

static AX_VOID EZoomAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    // AX_U8 nSnsId = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
    // AX_U8 nPrevChn = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_CHANNEL, "0"));
    static AX_BOOL bActionProcessing = AX_FALSE;
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    if (!bActionProcessing) {
        bActionProcessing = AX_TRUE;
        YieldProcessWebOpr(conn, E_REQ_TYPE_EZOOM, NULL, AX_FALSE);
        bActionProcessing = AX_FALSE;
    }
    ResponseStatusCode(conn, nHttpStatusCode);
}

static AX_VOID WSAudioAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        ResponseUnauthorized(conn);
        return;
    }
    SaveWSConnection(conn, 0, g_nAudioChannel);
}

static AX_VOID WSTalkAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        ResponseUnauthorized(conn);
        return;
    }
    SaveWSConnection(conn, 0, g_nTalkChannel);
}

static AX_VOID WSEventsAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_FALSE)) {
        ResponseUnauthorized(conn);
        return;
    }
    SaveWSConnection(conn, 0, g_nEventChannel);
}

static AX_VOID AssistInfoAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }

    if (strcmp(conn->rx->method, "GET") == 0) {
        AX_U8 nSnsId = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
        AX_U8 nPrevId = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_CHANNEL, "0"));

        AX_CHAR szResolution[16] = {0};

        AX_S32 nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_ASSIST, (AX_VOID**)&szResolution[0], AX_FALSE);
        cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
        cJSON* data = cJSON_GetObjectItem(pResponseBody, "data");
        if (nHttpStatusCode == RESPONSE_STATUS_OK_CODE) {
            cJSON_AddStringToObject(data, "assist_res", szResolution);
        } else {
            cJSON_AddStringToObject(data, "assist_res", "--");
        }

        AX_U8 nUniChn = GetUniChn(nSnsId, nPrevId);
        if (nUniChn != WEB_INVLID_ID) {
            AX_CHAR szBitrate[16] = {0};
            AX_F64 fBps = GetVencStatBitrate(nUniChn);
            sprintf(szBitrate, "%.2fkbps",fBps);
            cJSON_AddStringToObject(data, "assist_bitrate", szBitrate);
        } else {
            cJSON_AddStringToObject(data, "assist_bitrate", "--");
        }
        ResponseStatusCodeWithJson(conn, nHttpStatusCode, pResponseBody);
    }
}

static AX_VOID SystemAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    LOG_M_C(WEB, "...");
    if (strcmp(conn->rx->method, "GET") == 0) {
        AX_CHAR szTitle[64] = {0};
        sprintf(szTitle, "%s", "OPALDemo-IPC");
        AX_S32 nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_SYSTEM, (AX_VOID**)&szTitle[0], AX_FALSE);
        cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
        cJSON* data = cJSON_GetObjectItem(pResponseBody, "data");
        cJSON_AddStringToObject(data, PARAM_KEY_APP_NAME, szTitle);
        cJSON_AddStringToObject(data, PARAM_KEY_APP_VERSION, "1.0");
        cJSON_AddStringToObject(data, PARAM_KEY_SDK_VERSION, APP_BUILD_VERSION);
        ResponseStatusCodeWithJson(conn, nHttpStatusCode, pResponseBody);
    }
}

static AX_VOID CameraAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    LOG_M_C(WEB, "...");
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    static AX_BOOL bActionProcessing = AX_FALSE; /* Avoiding multiple web operations */
    if (strcmp(conn->rx->method, "GET") == 0) {
        cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
        cJSON* data = cJSON_GetObjectItem(pResponseBody, "data");
        AX_U8 nSnsId = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
        cJSON_AddItemToObject(data, "camera_attr", GetCameraJson(nSnsId));
        cJSON_AddItemToObject(data, "framerate_opts", GetSnsFramerateJson(nSnsId));
        cJSON_AddItemToObject(data, "resolution_opts", GetSnsResolutionJson(nSnsId));
        ResponseStatusCodeWithJson(conn, nHttpStatusCode, pResponseBody);
    } else {
        if (!bActionProcessing) {
            bActionProcessing = AX_TRUE;
            nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_CAMERA, NULL, AX_TRUE);
            ResponseStatusCode(conn, nHttpStatusCode);

            bActionProcessing = AX_FALSE;
        } else if (bActionProcessing) {
            // Response status: Not Acceptable
            nHttpStatusCode = RESPONSE_STATUS_FAILURE_CODE;
            ResponseStatusCode(conn, nHttpStatusCode);
        }
    }
}

static AX_VOID ImageAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    LOG_M_C(WEB, "...");
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    static AX_BOOL bActionProcessing = AX_FALSE; /* Avoiding multiple web operations */

    if (strcmp(conn->rx->method, "GET") == 0) {
        cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
        cJSON* data = cJSON_GetObjectItem(pResponseBody, "data");
        AX_U8 nSnsId = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));

        cJSON_AddItemToObject(data, "image_attr", GetImageJson(nSnsId));
        cJSON_AddItemToObject(data, "ldc_attr", GetLdcJson(nSnsId));
        cJSON_AddItemToObject(data, "dis_attr",  GetDisJson(nSnsId));
        ResponseStatusCodeWithJson(conn, nHttpStatusCode, pResponseBody);
    } else {
        if (!bActionProcessing) {
            bActionProcessing = AX_TRUE;
            nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_IMAGE, NULL, AX_TRUE);
            ResponseStatusCode(conn, nHttpStatusCode);
            bActionProcessing = AX_FALSE;
        } else {
            // Response status: Not Acceptable
            nHttpStatusCode = RESPONSE_STATUS_FAILURE_CODE;
            ResponseStatusCode(conn, nHttpStatusCode);
        }
    }
}

static AX_VOID AudioAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    LOG_M_C(WEB, "...");
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    if (strcmp(conn->rx->method, "GET") == 0) {
        cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
        cJSON_AddItemToObject(cJSON_GetObjectItem(pResponseBody, "data"), "info", GetAudioJson());
        ResponseStatusCodeWithJson(conn, nHttpStatusCode, pResponseBody);
    } else {
        nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_AUDIO, NULL, AX_TRUE);
        ResponseStatusCode(conn, nHttpStatusCode);
    }
}

static AX_VOID VideoAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    LOG_M_C(WEB, "...");
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    if (strcmp(conn->rx->method, "GET") == 0) {

        AX_U8 nSnsId = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
        if (nSnsId >= MAX_PREV_SNS_CHN_NUM) {
            nHttpStatusCode = RESPONSE_STATUS_FAILURE_CODE;
            ResponseStatusCode(conn, nHttpStatusCode);
            return;
        }
        cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
        cJSON* data = cJSON_GetObjectItem(pResponseBody, "data");

        AX_CHAR szCapList[16] = {0};
        AX_U8 nSize = g_stWsInfo.nSnsPrevChnCount[nSnsId];

        if (1 == nSize) {
            sprintf(szCapList, "[1,0,0,0]");
        } else if (2 == nSize) {
            sprintf(szCapList, "[1,1,0,0]");
        } else if (3 == nSize) {
            sprintf(szCapList, "[1,1,1,0]");
        } else if (4 == nSize) {
            sprintf(szCapList, "[1,1,1,1]");
        }

        cJSON_AddItemToObject(data, "cap_list", cJSON_Parse(szCapList));
        for (AX_U8 i = 0; i < nSize; i++) {
            AX_U8 nSrcChn = GetSrcChn(nSnsId, i);
            AX_U8 nUniChn = GetUniChn(nSnsId, i);
            AX_CHAR strKey[32] = {0};
            sprintf(strKey, "video%d", i);
            AX_U32 nWFisrt = g_pChnList[nUniChn]->nMaxWidth;
            AX_U32 nHFisrt = g_pChnList[nUniChn]->nMaxHeight;
            AX_U32 nWLast = 0;
            AX_U32 nHLast = 0;
            if ((i+1) < nSize) {
                AX_U8 nUniChnNext = GetUniChn(nSnsId, i+1);
                nWLast = g_pChnList[nUniChnNext]->nMaxWidth;
                nHLast = g_pChnList[nUniChnNext]->nMaxHeight;
            }
            cJSON_AddItemToObject(data, strKey, GetVideoJson(nSnsId, nSrcChn));

            sprintf(strKey, "attrVideo%d", i);
            cJSON_AddItemToObject(data, strKey, GetVideoAttrJson(nSnsId, nSrcChn, nWFisrt, nHFisrt, nWLast, nHLast));
        }
        ResponseStatusCodeWithJson(conn, nHttpStatusCode, pResponseBody);
    } else {
        nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_VIDEO, NULL, AX_TRUE);
        ResponseStatusCode(conn, nHttpStatusCode);
    }
}

static AX_VOID AiAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    LOG_M_C(WEB, "...");
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    AX_U8 nSnsId = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
    if (strcmp(conn->rx->method, "GET") == 0) {
        cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
        cJSON* data = cJSON_GetObjectItem(pResponseBody, "data");
        cJSON_AddItemToObject(data, "ai_attr",GetAiInfoJson(nSnsId));
        ResponseStatusCodeWithJson(conn, nHttpStatusCode,pResponseBody);
    } else {
        nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_AI, NULL, AX_TRUE);
        ResponseStatusCode(conn, nHttpStatusCode);
    }
}

static AX_VOID OverlayAction(HttpConn* conn) {
    if (!IsAuthorized(conn, AX_TRUE)) {
        ResponseUnauthorized(conn);
        return;
    }
    LOG_M_C(WEB, "...");
    AX_S32 nHttpStatusCode = RESPONSE_STATUS_OK_CODE;
    if (strcmp(conn->rx->method, "GET") == 0) {
        AX_U8 nSnsId = atoi(httpGetParam(conn, PARAM_KEY_PREVIEW_SNS_SRC, "0"));
        cJSON* pResponseBody = ConstructBaseResponse(RESPONSE_STATUS_OK_CODE, 0);
        cJSON* data = cJSON_GetObjectItem(pResponseBody, "data");

        cJSON_AddItemToObject(data, "overlay_attr", GetOsdJson(nSnsId));
        cJSON_AddItemToObject(data, "privacy_attr", GetPrivJson(nSnsId));

        ResponseStatusCodeWithJson(conn, nHttpStatusCode, pResponseBody);
    } else {
        nHttpStatusCode = YieldProcessWebOpr(conn, E_REQ_TYPE_OSD, NULL, AX_TRUE);
        ResponseStatusCode(conn, nHttpStatusCode);
    }
}

static AX_VOID ReportHttpStatus(AX_VOID) {
    LOG_M_C(WEB, "%s", httpStatsReport(HTTP_STATS_ALL));
}

static AX_VOID SendWSData(AX_VOID) {
    AX_U8 nSnsId = 0;
    AX_U8 nUniChn = 0;
    AX_BOOL arrDataStatus[MAX_WS_CONN_NUM] = {AX_FALSE};
    AX_BOOL arrConnStatus[MAX_WS_CONN_NUM] = {AX_FALSE};
    HttpConn* client = NULL;

    mprLock(g_pClients->mutex);
    for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
            continue;
        }

        nSnsId = GetSnsIDFromWS(client);
        if (nSnsId == WEB_INVLID_ID || nSnsId >= MAX_PREV_SNS_NUM) {
            continue;
        }

        nUniChn = GetUniChnFromWS(client);
        if (nUniChn == WEB_INVLID_ID || nUniChn >= MAX_WS_CONN_NUM) {
            continue;
        }

        arrConnStatus[nUniChn] = AX_TRUE;

        if (!g_pChnList[nUniChn]->hRingFifo) {
            continue;
        }

        AX_RINGFIFO_ELEMENT_T stData = {0};
        AX_S32 nRet = AX_RingFifo_Get(g_pChnList[nUniChn]->hRingFifo, &stData);

        if (0 != nRet) {
            continue;
        }

        //LOG_M_E(WEB, "nSnsId=%d,nSrcChn=%d,nUniChn=%d,data=%p, size=%d...", nSnsId, g_pChnList[nUniChn]->nSrcChn,nUniChn,stData.data[0].buf,stData.data[0].len);
        arrDataStatus[nUniChn] = AX_TRUE;  // got data

        WS_MSG_T* pMsg = GetWsMsgData(nUniChn);
        if (pMsg) {
            pMsg->conn = client;
            pMsg->packet = stData;
            pMsg->nUniChn = nUniChn;
            MprEvent* pEvent = mprCreateEvent(client->dispatcher, "ws", 0, (AX_VOID*)SendHttpData, (AX_VOID*)pMsg, MPR_EVENT_STATIC_DATA | MPR_EVENT_ALWAYS);
            if (!pEvent) {
                AX_RingFifo_Free(g_pChnList[nUniChn]->hRingFifo, &stData, AX_FALSE);
                ReleaseWsMsgData(pMsg);
            }
        } else {
            LOG_M_E(WEB, "sns=%d, uni=%d, no msg data space", nSnsId, nUniChn);
        }

    }
    mprUnlock(g_pClients->mutex);

    for (AX_U32 i = 0; i < MAX_WS_CONN_NUM; i++) {
        if (arrDataStatus[i] && g_pChnList[i]->hRingFifo) {
            AX_RingFifo_Pop(g_pChnList[i]->hRingFifo, AX_FALSE);
        }
    }
    // update connect status
    pthread_mutex_lock(&g_mtxConnStatus);
    for (AX_U32 i = 0; i < MAX_WS_CONN_NUM; ++i) {
        g_pChnList[i]->bConnected = arrConnStatus[i];
    }
    pthread_mutex_unlock(&g_mtxConnStatus);
}

static AX_VOID SendDataThreadFunc(AX_VOID* pThis) {
    prctl(PR_SET_NAME, "APP_WEB_Send");
    while (WS_IsRunning()) {
        SendWSData();
        OPAL_mSleep(10);
    }
}

static AX_VOID StatusCheckThreadFunc(AX_VOID* pThis) {
    prctl(PR_SET_NAME, "APP_WEB_Mem");
    AX_U32 nTimeout = 60000;

    pthread_condattr_t cattr;
    pthread_condattr_init(&cattr);
    pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
    pthread_cond_init(&g_cvStatusCheck, &cattr);

    while (g_stWsInfo.bServerStarted && g_stWsInfo.bStatusCheckStarted) {

        pthread_mutex_lock(&g_mtxStatusCheck);
        struct timespec tv;
        clock_gettime(CLOCK_MONOTONIC, &tv);
        tv.tv_sec += nTimeout / 1000;
        tv.tv_nsec += (nTimeout  % 1000) * 1000000;
        if (tv.tv_nsec >= 1000000000) {
            tv.tv_nsec -= 1000000000;
            tv.tv_sec += 1;
        }
        pthread_cond_timedwait(&g_cvStatusCheck, &g_mtxStatusCheck, &tv);
        pthread_mutex_unlock(&g_mtxStatusCheck);

        if (!g_stWsInfo.bServerStarted || !g_stWsInfo.bStatusCheckStarted) {
            break;
        }

        mprLock(g_pClients->mutex);
        // recovery RequestTimeout
        if (g_web_mem_limit_notified) {
            MprMemStats *ap = mprGetMemStats();
            HttpConn* client = NULL;
            const MprTicks RequestTimeout = 432000000; // 5days

            if (ap && ap->bytesAllocated > ap->bytesFree) {
                uint64 heapUsed = ap->bytesAllocated - ap->bytesFree;

                if (heapUsed < ap->warnHeap) {
                    for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
                        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
                            continue;
                        }
                        httpSetTimeout(client, RequestTimeout, -1);
                    }
                    g_web_mem_limit_notified = AX_FALSE;
                }
            }
        }
        mprUnlock(g_pClients->mutex);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
typedef struct _HTTP_ACTION_INFO {
    cchar* name;
    HttpAction action;
} HTTP_ACTION_INFO;

const HTTP_ACTION_INFO g_httpActionInfo[] = {{"/action/login", LoginAction},
                                             {"/action/setting/capability", CapabilityAction},
                                             {"/action/preview/assist", AssistInfoAction},
                                             {"/action/setting/system", SystemAction},
                                             {"/action/setting/camera", CameraAction},
                                             {"/action/setting/image", ImageAction},
                                             {"/action/setting/audio", AudioAction},
                                             {"/action/setting/video", VideoAction},
                                             {"/action/setting/ai", AiAction},
                                             {"/action/setting/overlay", OverlayAction},
                                             {"/action/preview/info", PreviewInfoAction},
                                             {"/action/preview/snapshot", SnapshotAction},
                                             {"/action/preview/ezoom", EZoomAction},
                                             {"/audio_0", WSAudioAction},
                                             {"/talk", WSTalkAction},
                                             {"/preview", WSPreviewAction},
                                             {"/pushimage", WSPushImageAction},
                                             {"/snapshot", WSSnapshotAction},
                                             {"/events", WSEventsAction}};


static AX_VOID WebServerThreadFunc(AX_VOID* pThis) {
    LOG_M_C(WEB, "+++");
    prctl(PR_SET_NAME, "APP_WEB_Server");
    Mpr* pMpr = NULL;
    do {
        if ((pMpr = mprCreate(0, NULL, MPR_USER_EVENTS_THREAD)) == 0) {
            LOG_M_E(WEB, "Cannot create runtime.");
            break;
        }

        if (httpCreate(HTTP_CLIENT_SIDE | HTTP_SERVER_SIDE) == 0) {
            LOG_M_E(WEB, "Cannot create the HTTP services.");
            break;
        }

        if (maLoadModules() < 0) {
            LOG_M_E(WEB, "Cannot load modules.");
            break;
        }

        g_pClients = mprCreateList(0, 0);
        mprAddRoot(g_pClients);
        mprStart();

        if (maParseConfig("./config/appweb.conf") < 0) {
            LOG_M_E(WEB, "load config failed.");
            break;
        }

        for (AX_U32 i = 0; i < sizeof(g_httpActionInfo) / sizeof(HTTP_ACTION_INFO); i++) {
            httpDefineAction(g_httpActionInfo[i].name, g_httpActionInfo[i].action);
        }

        if (httpStartEndpoints() < 0) {
            break;
        }

        g_stWsInfo.bServerStarted = AX_TRUE;
        g_stWsInfo.pSendDataThread = OPAL_CreateThread(SendDataThreadFunc, NULL);
        OPAL_StartThread(g_stWsInfo.pSendDataThread);

        g_stWsInfo.bStatusCheckStarted = AX_TRUE;
        g_stWsInfo.pStatucCheckThread = OPAL_CreateThread(StatusCheckThreadFunc, NULL);
        OPAL_StartThread(g_stWsInfo.pStatucCheckThread);

        mprSetMemNotifier(WebServerMemNotifier);

        if (IsEnableWebServerStatusCheck()) {
            mprCreateTimerEvent(NULL, "HttpStatusReport", 1000 * 60, (AX_VOID*)ReportHttpStatus, NULL, MPR_EVENT_CONTINUOUS | MPR_EVENT_QUICK);
        }

        mprServiceEvents(-1, 0);
    } while (0);

    //LOG_M_I(WEB, "---");
}

static AX_S32 FormatEvent2Json(WEB_EVENTS_DATA_T* pEvent, AX_CHAR* szEventJson) {
    AX_CHAR szDate[64] = {0};
    OPAL_GetLocalTime(szDate, 16, ':', AX_FALSE);
    return snprintf(szEventJson, MAX_EVENTS_CHN_SIZE, "{\"events\": [{\"type\": %d, \"sensor\":%d, \"date\": \"%s\"}]}", pEvent->eType,
             pEvent->nReserved, szDate);
}

static AX_BOOL SendLogOutData() {
    if (!WS_IsRunning()) {
        return AX_FALSE;
    }
    AX_BOOL bRet = AX_FALSE;
    HttpConn* client = NULL;
    AX_U8 nUniChn = 0;
    mprLock(g_pClients->mutex);
    for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
            continue;
        }
        nUniChn = GetUniChnFromWS(client);
        if (nUniChn == g_nEventChannel) {
            WEB_EVENTS_DATA_T tEvent = {0};
            tEvent.eType = E_WEB_EVENTS_TYPE_LogOut;
            AX_CHAR szEventJson[MAX_EVENTS_CHN_SIZE] = {0};
            AX_U32 len = FormatEvent2Json(&tEvent, szEventJson);
            //LOG_M_D(WEB, "Send logout data to client=%p", client);
            ssize nRet = httpSendBlock(client, WS_MSG_BINARY, szEventJson, len, HTTP_BLOCK);
            if (nRet == len) {
                bRet = AX_TRUE;
            }
        }
    }
    mprUnlock(g_pClients->mutex);
    g_stWsInfo.bServerStarted = AX_FALSE;

    if (bRet) {
        OPAL_mSleep(1000);  // wait for web log out
    }
    //LOG_M_I(WEB, "Send logout data %s.", bRet ? "success" : "failed");
    return bRet;
}

AX_BOOL WS_Init() {
    char szName[64] = {0};
    AX_U32 nAlgoImgW = 1280;
    AX_U32 nAlgoImgH = 720;
    for(AX_S32 i=0; i < MAX_PREV_SNS_NUM; i++) {
        if (g_stOpalAttr.stVideoAttr[i].bEnable) {
            AX_U8 nPrevChn = 0;
            for (AX_U32 j = 0; j < MAX_PREV_SNS_CHN_NUM; j++) {
                if (g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].bEnable
                    && (AX_OPAL_VIDEO_CHN_TYPE_H264 == g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].eType
                    || AX_OPAL_VIDEO_CHN_TYPE_H265 == g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].eType
                    || AX_OPAL_VIDEO_CHN_TYPE_MJPEG == g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].eType)) {

                    AX_OPAL_VIDEO_CHN_ATTR_T stChnAttr;
                    AX_OPAL_Video_GetChnAttr(i, j, &stChnAttr);

                    g_stWsInfo.stChnVideo[i][j].nSnsId = i;
                    g_stWsInfo.stChnVideo[i][j].nPrevChn = nPrevChn;
                    g_stWsInfo.stChnVideo[i][j].nSrcChn = j;
                    g_stWsInfo.stChnVideo[i][j].bValid = 1;
                    g_stWsInfo.stChnVideo[i][j].bConnected = 0;
                    g_stWsInfo.stChnVideo[i][j].nMaxWidth = stChnAttr.nMaxWidth;
                    g_stWsInfo.stChnVideo[i][j].nMaxHeight = stChnAttr.nMaxHeight;
                    pthread_mutex_init(&g_stWsInfo.stChnVideo[i][j].mtxWsMsg, NULL);

                    sprintf(szName, "VIDEO_%d_%d", i, nPrevChn);
                    AX_U32 nRingBufSize = GetVencRingBufSize(g_stWsInfo.stChnVideo[i][j].nMaxWidth, g_stWsInfo.stChnVideo[i][j].nMaxHeight);
                    AX_RingFifo_Init(&g_stWsInfo.stChnVideo[i][j].hRingFifo, nRingBufSize, szName);
                    nPrevChn ++;
                    g_stWsInfo.nSnsPrevChnCount[i] = nPrevChn;

                } else {
                    g_stWsInfo.stChnVideo[i][j].nSnsId = i;
                    g_stWsInfo.stChnVideo[i][j].nPrevChn = WEB_INVLID_ID;
                    g_stWsInfo.stChnVideo[i][j].nSrcChn = WEB_INVLID_ID;
                    g_stWsInfo.stChnVideo[i][j].bValid = 0;
                    g_stWsInfo.stChnVideo[i][j].bConnected = 0;
                    if (g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].eType == AX_OPAL_VIDEO_CHN_TYPE_ALGO) {
                        if (g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].nMaxWidth > nAlgoImgW ||
                            g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].nMaxHeight > nAlgoImgH) {
                            nAlgoImgW = g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].nMaxWidth;
                            nAlgoImgH = g_stOpalAttr.stVideoAttr[i].stPipeAttr.stVideoChnAttr[j].nMaxHeight;
                        }
                    }
                }
            }
            g_stWsInfo.nSnsCount++;
        }
    }

    AX_U32 nImgRingBufSize = (AX_U32)(nAlgoImgW * nAlgoImgH * 3.0 / 2 * GetJencOutBuffRatio() * GetWebJencRingBufCount());
    AX_U32 nAudioRingBufSize = GetAencOutFrmSize() * GetWebAencRingBufCount();

    g_stWsInfo.nSnsCount = g_opal_nSnsNum;
    AX_RingFifo_Init(&g_stWsInfo.stChnEvent.hRingFifo, MAX_EVENTS_CHN_SIZE * GetWebEventsRingBufCount(), "EVENTS");
    AX_RingFifo_Init(&g_stWsInfo.stChnImage.hRingFifo, nImgRingBufSize, "IMAGE");
    AX_RingFifo_Init(&g_stWsInfo.stChnAudio.hRingFifo, nAudioRingBufSize, "AUDIO");

    pthread_mutex_init(&g_stWsInfo.stChnEvent.mtxWsMsg, NULL);
    pthread_mutex_init(&g_stWsInfo.stChnImage.mtxWsMsg, NULL);
    pthread_mutex_init(&g_stWsInfo.stChnAudio.mtxWsMsg, NULL);

    for(AX_S32 i = 0; i < MAX_PREV_SNS_NUM; i++) {
        for (AX_U32 j = 0; j < MAX_PREV_SNS_CHN_NUM; j++) {
            g_pChnList[i*MAX_PREV_SNS_CHN_NUM +j] = &g_stWsInfo.stChnVideo[i][j];
            g_pChnList[i*MAX_PREV_SNS_CHN_NUM +j]->nUniChn = (AX_U8)(i*MAX_PREV_SNS_CHN_NUM +j);
        }
    }

    g_pChnList[g_nEventChannel]     = &g_stWsInfo.stChnEvent;
    g_pChnList[g_nPushImgChannel]     = &g_stWsInfo.stChnImage;
    g_pChnList[g_nAudioChannel]     = &g_stWsInfo.stChnAudio;
    g_pChnList[g_nSnapshotChannel]  = &g_stWsInfo.stChnSnapshot;
    g_pChnList[g_nTalkChannel]      = &g_stWsInfo.stChnTalk;

    g_pChnList[g_nEventChannel]->nUniChn    = g_nEventChannel;
    g_pChnList[g_nPushImgChannel]->nUniChn    = g_nPushImgChannel;
    g_pChnList[g_nAudioChannel]->nUniChn    = g_nAudioChannel;
    g_pChnList[g_nSnapshotChannel]->nUniChn = g_nSnapshotChannel;
    g_pChnList[g_nTalkChannel]->nUniChn     = g_nTalkChannel;

    g_mapUser2Token = ax_map_create(ax_map_type_ss);

    return AX_TRUE;
}

AX_BOOL WS_DeInit() {
    for(AX_S32 i=0; i < AX_OPAL_SNS_ID_BUTT; i++) {
        for (AX_U32 j = 0; j < MAX_PREV_SNS_CHN_NUM; j++) {
            if (g_stWsInfo.stChnVideo[i][j].hRingFifo) {
                AX_RingFifo_Deinit(g_stWsInfo.stChnVideo[i][j].hRingFifo);
            }
        }
    }
    if (g_mapUser2Token) {
        ax_map_destory(g_mapUser2Token);
    }
    return AX_TRUE;
}

AX_BOOL WS_Start() {
    g_stWsInfo.pAppwebThread = OPAL_CreateThread(WebServerThreadFunc, NULL);
    if (g_stWsInfo.pAppwebThread) {
        OPAL_StartThread(g_stWsInfo.pAppwebThread);
    }
    return (g_stWsInfo.pAppwebThread) ? AX_TRUE : AX_FALSE;
}

AX_BOOL WS_Stop() {

    SendLogOutData();

    pthread_mutex_lock(&g_mtxStatusCheck);
    g_stWsInfo.bStatusCheckStarted = AX_FALSE;
    pthread_cond_broadcast(&g_cvStatusCheck);
    pthread_mutex_unlock(&g_mtxStatusCheck);

    if (g_stWsInfo.pSendDataThread) {
        OPAL_StopThread(g_stWsInfo.pSendDataThread);
        OPAL_DestroyThread(g_stWsInfo.pSendDataThread);
        g_stWsInfo.pSendDataThread = NULL;
    }

    if (g_stWsInfo.pStatucCheckThread) {
        OPAL_StopThread(g_stWsInfo.pStatucCheckThread);
        OPAL_DestroyThread(g_stWsInfo.pStatucCheckThread);
        g_stWsInfo.pStatucCheckThread = NULL;
    }

    if (g_stWsInfo.pAppwebThread) {
        g_stWsInfo.bServerStarted = AX_FALSE;
        mprShutdown(MPR_EXIT_NORMAL, -1, MPR_EXIT_TIMEOUT);
        OPAL_StopThread(g_stWsInfo.pAppwebThread);
        OPAL_DestroyThread(g_stWsInfo.pAppwebThread);
        g_stWsInfo.pAppwebThread = NULL;
    }
    return AX_TRUE;
}

AX_VOID WS_SendPreviewData(AX_U8 nSnsId, AX_U8 nSrcChn, AX_U8* pData, AX_U32 size, AX_U64 u64Pts, AX_BOOL bIFrame) {
    if (!WS_IsRunning()) {
        return;
    }
    AX_U8 nUniChn = GetUniChnBySrcChn(nSnsId, nSrcChn);
    if (nUniChn == WEB_INVLID_ID) {
        return;
    }

    UpdateVencStat(nUniChn, size);

    if(!g_pChnList[nUniChn]->bConnected) {
        return;
    }

    AX_RINGFIFO_HANDLE hRingFifo = g_pChnList[nUniChn]->hRingFifo;
    if (!hRingFifo) {
        LOG_M_E(WEB, "sns=%d, src=%d, no ringfifo", nSnsId, nSrcChn);
        return;
    }

    PTS_HEADER_T tHeader;
    tHeader.nMagic = PTS_MAGIC;
    tHeader.nDatalen = size;
    tHeader.nPts = u64Pts;
    if (0 != AX_RingFifo_Put(hRingFifo, pData, size, u64Pts, bIFrame, (AX_U8*)&tHeader, (AX_U32)(sizeof(tHeader)))) {
        mprLock(g_pClients->mutex);
        if (g_pChnList[nUniChn]->nWsMsgUsed == 0) {
            LOG_M_E(WEB, "sns=%d, src=%d, clear ringbuffer", nSnsId, nSrcChn);
            AX_RingFifo_Clear(hRingFifo);
        }
        mprUnlock(g_pClients->mutex);
        if (0 != AX_RingFifo_Put(hRingFifo, pData, size, u64Pts, bIFrame, (AX_U8*)&tHeader, (AX_U32)(sizeof(tHeader)))) {
            LOG_M_E(WEB, "sns=%d, src=%d put failed", nSnsId, nSrcChn);
        }
    }
}

AX_VOID WS_SendPushImgData(AX_U8 nSnsId, AX_VOID* data, AX_U32 size, AX_U64 nPts, AX_BOOL bIFrame, JPEG_DATA_INFO_T* pJpegInfo) {
    if (!WS_IsRunning()) {
        return;
    }

    /* Waiting for reading thread to refresh the websock conn status */
    if(!g_stWsInfo.stChnImage.bConnected) {
        return;
    }


    if (NULL != pJpegInfo) {
        AX_CHAR szJsonData[256] = {0};
        /* construct json data info */
        if (pJpegInfo->eType == JPEG_TYPE_PLATE) {
            sprintf(szJsonData, "{\"snsId\": %d, \"type\": %d, \"attribute\": {\"plate\": {\"number\": \"%s\", \"color\": \"%s\"} }}",
                    nSnsId, pJpegInfo->eType, pJpegInfo->tPlateInfo.szNum, pJpegInfo->tPlateInfo.szColor);
        } else if (pJpegInfo->eType == JPEG_TYPE_FACE) {
            sprintf(szJsonData,
                    "{\"snsId\": %d, \"type\": %d, \"attribute\": {\"face\": {\"gender\": %d, \"age\": %d, \"mask\": \"%s\", \"info\": "
                    "\"%s\"}}}",
                    nSnsId, pJpegInfo->eType, pJpegInfo->tFaceInfo.nGender, pJpegInfo->tFaceInfo.nAge, pJpegInfo->tFaceInfo.szMask,
                    pJpegInfo->tFaceInfo.szInfo);
        } else {
            sprintf(szJsonData, "{\"snsId\": %d, \"type\": %d, \"attribute\": {\"src\": %d, \"width\": %d, \"height\": %d}}", nSnsId,
                    pJpegInfo->eType, pJpegInfo->tCaptureInfo.tHeaderInfo.nSnsSrc, pJpegInfo->tCaptureInfo.tHeaderInfo.nWidth,
                    pJpegInfo->tCaptureInfo.tHeaderInfo.nHeight);
        }

        AX_U32 nJsnLen = strlen(szJsonData);

        /* construct jpeg head */
        JPEG_HEAD_T stJpegHead = INIT_JPEG_HEAD;
        stJpegHead.nJsonLen = nJsnLen > 0 ? nJsnLen + 1 : 0;
        stJpegHead.nTotalLen = 4 /*magic*/ + 4 /*total len*/ + 4 /*tag*/ + 4 /*json len*/ + stJpegHead.nJsonLen;

        strcpy(stJpegHead.szJsonData, szJsonData);

        AX_RingFifo_Put(g_stWsInfo.stChnImage.hRingFifo, data, size, nPts, bIFrame, (AX_U8*)&stJpegHead, stJpegHead.nTotalLen);
    } else {
        AX_RingFifo_Put(g_stWsInfo.stChnImage.hRingFifo, data, size, nPts, bIFrame, NULL, 0);
    }
}

AX_VOID WS_SendSnapshotData(AX_VOID* data, AX_U32 size, AX_VOID* conn) {
    /* mprResetYield is needed when this function works as callback of SnapshotAction */
    // mprResetYield();

    if (!WS_IsRunning()) {
        return;
    }

    HttpConn* client = NULL;
    cchar* szHttpToken = GetTokenFromConn((HttpConn*)conn, AX_TRUE);
    // no mprLock(g_pClients->mutex) because sync call by web server snapshot command.
    for (AX_S32 next = 0; (client = (HttpConn*)MprListGetNextItem(g_pClients, &next)) != 0;) {
        if (WS_STATE_OPEN != httpGetWebSocketState(client)) {
            continue;
        }

        AX_U8 nUniChn = GetUniChnFromWS(client);
        if (nUniChn != g_nSnapshotChannel) {
            continue;
        }

        cchar* szWSToken = GetTokenFromConn(client, AX_FALSE);
        if ((szWSToken == NULL) ||
            (strcmp(szWSToken, szHttpToken) != 0)) {
            continue;
        }

        ssize nRet = httpSendBlock(client, WS_MSG_BINARY, (cchar*)data, size, HTTP_BLOCK);
        if (nRet >= 0) {
            break;
        }
    }
}

AX_VOID WS_SendAudioData(AX_VOID* data, AX_U32 size, AX_U64 nPts /*= 0*/) {
    // AAC/PCM711A
    if (!WS_IsRunning()) {
        return;
    }

    if(!g_stWsInfo.stChnAudio.bConnected) {
        return;
    }

    PTS_HEADER_T tHeader;
    tHeader.nMagic = PTS_MAGIC;
    tHeader.nDatalen = size;
    tHeader.nPts = nPts;
    AX_RingFifo_Put(g_stWsInfo.stChnAudio.hRingFifo, data, size, nPts, AX_TRUE, (AX_U8*)&tHeader, (AX_U32)(sizeof(tHeader)));
}

AX_BOOL WS_SendEventsData(WEB_EVENTS_DATA_T* data) {
    if (!WS_IsRunning()) {
        return AX_FALSE;
    }

    if(!g_stWsInfo.stChnEvent.bConnected) {
        return AX_FALSE;
    }

    AX_CHAR szEventJson[MAX_EVENTS_CHN_SIZE] = {0};
    AX_U32 len = FormatEvent2Json(data, szEventJson);
    AX_RingFifo_Put(g_stWsInfo.stChnEvent.hRingFifo, szEventJson, len, g_nEventChannel, AX_TRUE, NULL, 0);

    return AX_TRUE;
}

AX_BOOL WS_SendWebResetEvent(AX_U8 nSnsId) {
    /*notify web restart preview*/
    WEB_EVENTS_DATA_T event;
    event.eType = E_WEB_EVENTS_TYPE_ReStartPreview;
    event.nReserved = nSnsId;
    WS_SendEventsData(&event);
    return AX_TRUE;
}

AX_VOID EnableAudioPlay(AX_BOOL bEnable) {
    g_stWsInfo.bAudioPlayAvailable = bEnable;
}

AX_VOID EnableAudioCapture(AX_BOOL bEnable) {
   g_stWsInfo.bAudioCaptureAvailable = bEnable;
}

AX_BOOL WS_IsRunning() {
    return g_stWsInfo.bServerStarted;
}
