/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "qsdemo.h"
#include "ax_model_api.h"
#include "ax_skel_api.h"
#include "ax_ives_api.h"
#include "AXRtspWrapper.h"
#include "qs_timer.h"
#include "qs_recorder.h"
#include "qs_utils.h"
#include "audio_config.h"
#include "ax_osd.h"

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <dirent.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

extern int malloc_trim(size_t pad);

static AX_VOID GetYuvForQRCode();

#define ALGO_IVPS_CHN (2)
#define JENC_OUT_CHN (2)

#define PINGPONG_BUF_NUM (1)

#define QSReboot  0
#define QSAOV     1

#define AX_EZOOM_MAX  16

#ifndef AX_ALIGN_UP
#define AX_ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#endif

#ifndef ALIGN_COMM_DOWN
#define ALIGN_COMM_DOWN(x, align) (((x) / (align)) * (align))
#endif

typedef struct {
    AX_U64                  u64DetectTimeFromBoot[2];
    AX_U64                  u64Venc0TimeFromBoot[2];
    AX_U64                  u64DetectYUVTimeFromBoot[2];

    AX_U64                  u64SysInit;
    AX_U64                  u64NpuInit;
    AX_U64                  u64VinInit;

    AX_U64                  u64CamOpen[2];
    AX_U64                  u64IvpsInit;
    AX_U64                  u64VencInit;
    AX_U64                  u64DetectInit;

    AX_U64                  u64CamOpenLastPts;
    AX_U64                  u64DoSleepPts;
} AX_LANUNCH_TIME_T;

typedef struct {
    AX_BOOL bEnable;
    AX_RTSP_HANDLE pRtspHandle;
} SAMPLE_RTSP_PARAM_T;

static AX_U64 g_u64LaunchPts = 0;
static AX_U8 g_nIgnoreLost[2] = {0, 0};
static AX_LANUNCH_TIME_T g_stResultTimes = {0};
static AX_BOOL g_bNeedShowLauchResult = AX_TRUE;
static AX_RTSP_HANDLE g_hRtsp = NULL;

static const AX_CHAR* gH264Path[2] = {"/tmp/venc0.264", "/tmp/venc1.264"};
static const AX_CHAR* gJpegPath[2] = {"/tmp/jenc0.jpg", "/tmp/jenc1.jpg"};


AX_S32 g_nRunningTimes = 0;
static AX_S32 gDetectFrames[2] = {0, 0};

static SAMPLE_ENTRY_PARAM_T g_entry_param = {
    .eHdrMode = QS_DEMO_DEFAULT_SNS_MODE,
    .nAiIspMode = E_SNS_AIISP_DEFAULT_SCENE_MODE_E,
    .bJencEnable = AX_TRUE,
    .bVencEnable = AX_TRUE,
    .bDetectEnable = AX_TRUE,
    .nSensorFrameRate = SENSOR_FRAMERATE,
    .nJencFrameRate = JENC_FRAMERATE,
    .nDetectFrameRate = DETECT_FRAMERATE,
    .nDetectFrameMax = DET_MAX_FRAME_NOBODY,
    .nVencDumpNum = 60,
    .bJencDump = AX_TRUE,
    .nIvpsChnCnt = IVPSChannelNumber,
    .nSleepTime = 800,   //ms
    .nDetetAlgo = 0,  // SKEL
    .nAudioFlag = 0,  // NONE
    .bSc850sl2M = AX_FALSE,
    .nHdrModeTest = 0, // disable
    .nVinIvpsMode = AX_ITP_ONLINE_VPP,
    .nGdcOnlineVppTest = 0,  // disable
    .nVinChnFrmMode = AX_VIN_FRAME_MODE_RING,
    .nRebootInterval = 0,
    .bRandomCapNum = AX_FALSE,
    .nCapNum = 0,
    .bRtspEnable = AX_TRUE,
    .nVencRcChangePolicy = 0, // 0: change bitrate, 1: change gop
    .nMaxRecordFileCount = MAX_RECORD_FILE_COUNT,
    .nGopInAov = 10,
    .nSwitchSnsId = 1,
    .bAutoZoomLoopOn = AX_FALSE,
    .tEzoomSwitchInfo.nX = 0,
    .tEzoomSwitchInfo.nY = 0,
    .tEzoomSwitchInfo.nWidth = 0,
    .tEzoomSwitchInfo.nHeight = 0,
    .tEzoomSwitchInfo.nCx = 0,
    .tEzoomSwitchInfo.nCy = 0,
    .tEzoomSwitchInfo.fSwitchRatio = 3.0,
    .nSlowShutter = -1,
    .nAeManualShutter = -1
};

typedef struct _IVPS_GET_THREAD_PARAM
{
    AX_U8 nIvpsGrp;
    AX_U8 nIvpsChn;
    pthread_t nTid;
} DETECT_GET_THREAD_PARAM_T;
static DETECT_GET_THREAD_PARAM_T g_DetectThrdParam[2] = {{0, ALGO_IVPS_CHN, 0}, {1, ALGO_IVPS_CHN, 0}};

static volatile AX_S32 gLoopExit = 0;
static AX_SKEL_HANDLE g_pSkelHandle = NULL;
static AX_BOOL g_NeedRebootSystem = AX_FALSE;
static AX_CAMERA_T gCams[MAX_CAMERAS] = {0};
extern SAMPLE_CHN_ATTR_T gOutChnAttr[];

static volatile AX_BOOL g_bThreadLoopExit;
pthread_mutex_t g_mutex;
static AX_BOOL  g_bSleepFlag = AX_FALSE;

static unsigned int g_conditions_sleep = CONDITION0_MASK_NONE
                                        | CONDITION_MASK_VENC0
                                        | CONDITION_MASK_VENC1
                                        | CONDITION_MASK_AUDIO
                                        | CONDITION_MASK_RECORD0
                                        | CONDITION_MASK_RECORD1;
static pthread_mutex_t g_mutex_sleep = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond_sleep = PTHREAD_COND_INITIALIZER;

#ifdef QSDEMO_AUDIO_SUPPORT
// audio flag
#define SAMPLE_AUDIO_FLAG_NONE    0x00
#define SAMPLE_AUDIO_FLAG_CAPTURE 0x01
#define SAMPLE_AUDIO_FLAG_PLAY    0x02
#define SAMPLE_AUDIO_FLAG_ALL     (SAMPLE_AUDIO_FLAG_CAPTURE | SAMPLE_AUDIO_FLAG_PLAY)

// audio file type
#define SAMPLE_AUDIO_FILE_TYPE_NONE 0
#define SAMPLE_AUDIO_FILE_TYPE_MONITOR 1

static AX_U32 g_sample_audio_file_type = SAMPLE_AUDIO_FILE_TYPE_NONE;
static pthread_t g_sample_audio_file_play_tid;
static pthread_mutex_t g_mutex_trigger_audio_file_play = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond_trigger_audio_file_play = PTHREAD_COND_INITIALIZER;

static AX_S32 SampleStartAudioFilePlay(AX_U32 nAudioType);
#endif

static AX_BOOL g_nLastFps = SENSOR_FRAMERATE;
static AX_S32  g_nQSType = -1;  // 0: QSReboot; 1: QSAOV

static VENC_GETSTREAM_PARAM_T gGetStreamParam[SAMPLE_VENC_CHN_NUM_MAX] = {0};

AX_BOOL  g_bIvpsInited = AX_FALSE;
pthread_mutex_t g_mtxIvpsInit = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_condIvpsInit = PTHREAD_COND_INITIALIZER;
AX_BOOL  g_bVinPostInited = AX_FALSE;

#if defined(AX_RISCV_LOAD_MODEL_SUPPORT)
static AX_U8 g_release_model_image_mem_flag = 0; // BIT0: CAM0, BIT1: CAM1
static pthread_mutex_t g_mtxReleaseImageMem = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_condReleaseImageMem = PTHREAD_COND_INITIALIZER;
#endif

#if defined(SAMPLE_SNS_SC200AI_DOUBLE) || defined(SAMPLE_SNS_SC850SL_OS04A10) || defined(SAMPLE_SNS_SC200AI_TRIPLE) || defined(SAMPLE_SNS_OS04A10_DOUBLE)
SYS_LINK_MOD_PARAM_T gLinkModAttr[] = {
    {.srcMod = {.enModId=AX_ID_VIN, .s32GrpId=0, .s32ChnId=0}, .dstMod = {.enModId=AX_ID_IVPS, .s32GrpId=0, .s32ChnId=0}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=0, .s32ChnId=0}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=0}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=0, .s32ChnId=1}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=1}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=0, .s32ChnId=2}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=2}},

    {.srcMod = {.enModId=AX_ID_VIN, .s32GrpId=1, .s32ChnId=0}, .dstMod = {.enModId=AX_ID_IVPS, .s32GrpId=1, .s32ChnId=0}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=1, .s32ChnId=0}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=3}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=1, .s32ChnId=1}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=4}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=1, .s32ChnId=2}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=5}},
};
#else
SYS_LINK_MOD_PARAM_T gLinkModAttr[] = {
    {.srcMod = {.enModId=AX_ID_VIN, .s32GrpId=0, .s32ChnId=0}, .dstMod = {.enModId=AX_ID_IVPS, .s32GrpId=0, .s32ChnId=0}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=0, .s32ChnId=0}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=0}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=0, .s32ChnId=1}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=1}},
    {.srcMod = {.enModId=AX_ID_IVPS, .s32GrpId=0, .s32ChnId=2}, .dstMod = {.enModId=AX_ID_VENC, .s32GrpId=0, .s32ChnId=2}},
};
#endif

#define SET_IGNORE(cam) g_nIgnoreLost[cam] = ((1 << 0) | (1 << 1))
#define RESET_IGNORE(cam, chn) g_nIgnoreLost[cam] ^= (1 << chn)
#define IS_IGNORE(cam, chn) ((g_nIgnoreLost[cam] & (1 << chn)))

static AX_VOID NoBodyFoundNotify();
static AX_BOOL CheckStopFlag();


#define MAIN_TRIM_THRESHOLD (128*1024)

static AX_VOID QS_SetMalloptPolicy(AX_VOID) {
    mallopt(M_TRIM_THRESHOLD, MAIN_TRIM_THRESHOLD);
}

AX_S32 DetectThreadStart(SAMPLE_ENTRY_PARAM_T *pEntryParam);

AX_VOID ThreadLoopStateSet(AX_BOOL bValue)
{
    g_bThreadLoopExit = bValue;
}

AX_BOOL ThreadLoopStateGet(AX_VOID)
{
    return g_bThreadLoopExit;
}

static AX_VOID QSClearVencSeqNum() {
    for (AX_S32 i = 0; i < SAMPLE_VENC_CHN_NUM_MAX; i++) {
        gGetStreamParam[i].u64LastSeqNum = 0;
    }
}

static int get_required_mask() {
    int sleep_flag = 0;

    sleep_flag |= CONDITION_MASK_SKEL;
    sleep_flag |= CONDITION_MASK_VENC0;
    sleep_flag |= CONDITION_MASK_RECORD0;

    if (g_entry_param.nCamCnt > 1) {
        sleep_flag |= CONDITION_MASK_VENC1;
        sleep_flag |= CONDITION_MASK_RECORD1;
    }

#ifdef QSDEMO_AUDIO_SUPPORT
    sleep_flag |= CONDITION_MASK_AUDIO;
#endif

    return sleep_flag;
}

static int check_sleep_condition() {
    int ret = 0;
    pthread_mutex_lock(&g_mutex_sleep);
    int required_mask = get_required_mask();
    while ((g_conditions_sleep & required_mask) != required_mask) {
        if ((g_conditions_sleep & CONDITION_MASK_STOP) == CONDITION_MASK_STOP) {
            ret = 1;
            break;
        }
        pthread_cond_wait(&g_cond_sleep, &g_mutex_sleep);
    }
    pthread_mutex_unlock(&g_mutex_sleep);
    return ret;
}

void update_sleep_condition(int mask, bool bSetTrue) {
    pthread_mutex_lock(&g_mutex_sleep);
    g_conditions_sleep = bSetTrue ? (g_conditions_sleep | mask) : (g_conditions_sleep & ~mask);
    int required_mask = get_required_mask();
    if ((g_conditions_sleep & required_mask) == required_mask
        || (g_conditions_sleep & CONDITION_MASK_STOP) == CONDITION_MASK_STOP) {
        pthread_cond_signal(&g_cond_sleep);
    }
    pthread_mutex_unlock(&g_mutex_sleep);
}

#ifdef QSDEMO_AUDIO_SUPPORT
AX_VOID AudioPlayCallback(const SAMPLE_AUDIO_PLAY_FILE_RESULT_PTR pstResult) {
    if (pstResult) {
        ALOGW("%s play complete, status: %d", pstResult->pstrFileName, pstResult->eStatus);
    }

    update_sleep_condition(CONDITION_MASK_AUDIO, true);
}
#endif

AX_S32 QSNotifyEventCallback(const AX_NOTIFY_EVENT_E event, AX_VOID * pdata) {
    if (event == AX_NOTIFY_EVENT_SLEEP) {
        AX_U64 u64CurPts = GetTickCountPts();
        pthread_mutex_lock(&g_mutex);

        update_sleep_condition(CONDITION_MASK_VENC0, false);
        update_sleep_condition(CONDITION_MASK_VENC1, false);
        update_sleep_condition(CONDITION_MASK_SKEL, false);

        QSClearVencSeqNum();
        pthread_mutex_unlock(&g_mutex);

        ALOGW("Sys Slept round[%d], timecost: %llu", g_nRunningTimes, u64CurPts - g_stResultTimes.u64DoSleepPts);

    } else if (event == AX_NOTIFY_EVENT_WAKEUP) {
        AX_OSD_UpdateImmediately();

        AX_U64 u64CurPts = GetTickCountPts();
        pthread_mutex_lock(&g_mutex);
        g_bSleepFlag = AX_FALSE;
        pthread_mutex_unlock(&g_mutex);
        LogToFile("Sys Wakeup round[%d], timecost: %llu", g_nRunningTimes, u64CurPts - g_stResultTimes.u64DoSleepPts);

        QS_VideoRecorderWakeup(AX_TRUE);

        ++g_nRunningTimes;
    }

    return 0;
}

static AX_VOID TestRandomCapNum() {
    AX_S32 nRet = 0;
    AX_S32 nCapNum = 0;
    AX_S32 i = 0;
    if (g_entry_param.bRandomCapNum) {
        if (g_nRunningTimes % 60 == 0) {
            srand(time(NULL));
            nCapNum = rand() % 3 + 1;
            for (i = 0; i < g_entry_param.nCamCnt; i++) {
                nRet = AX_VIN_SetCapFrameNumbers(gCams[i].nPipeId, nCapNum);
                if (nRet == AX_SUCCESS) {
                    ALOGI("AX_VIN_SetCapFrameNumbers: pipe=%d, num=%d.", gCams[i].nPipeId, nCapNum);
                } else {
                    ALOGE("AX_VIN_SetCapFrameNumbers: pipe=%d, num=%d, failed, error=0x%x", gCams[i].nPipeId, nCapNum, nRet);
                }
            }
            g_entry_param.nCapNum = nCapNum;
            g_entry_param.nDetectFrameMax = nCapNum;
        }
    }
}

static AX_VOID *sleep_thread_func(AX_VOID *param) {
    AX_S32 ret= 0;

    prctl(PR_SET_NAME, "qs_sleep");

    wakeup_timer_set(g_entry_param.nSleepTime);

    while (!gLoopExit) {

        // block until ready to sleep and saved
        ret = check_sleep_condition();
        if (gLoopExit || ret) {
            break;
        }

        g_entry_param.nDetectFrameMax = (g_entry_param.nCapNum == 0) ? 1 : g_entry_param.nCapNum;
        // for test
        TestRandomCapNum();

        QS_VideoRecorderWakeup(AX_FALSE);

        ALOGI("AX_SYS_Sleep and wakeup after %d ms...", g_entry_param.nSleepTime);
        g_stResultTimes.u64DoSleepPts = GetTickCountPts();

        // for debug: timestamp invoke sleep
        AX_SYS_SleepTimeStamp(AX_ID_USER, E_STS_SYSSLEEP_INVOKE);

        ret = AX_SYS_Sleep();

        // for debug: timestamp sleep return
        AX_SYS_SleepTimeStamp(AX_ID_USER, E_STS_SYSSLEEP_RETURN);

        if (ret != 0) {
            ALOGE("AX_SYS_Sleep failed, error: 0x%X", ret);
            QS_VideoRecorderWakeup(AX_TRUE);
            pthread_mutex_lock(&g_mutex);
            update_sleep_condition(CONDITION_MASK_SKEL, false);
            g_bSleepFlag = AX_FALSE;
            pthread_mutex_unlock(&g_mutex);
        }

        MSSleep(10);
    }

    return NULL;
}

static AX_S32 StartSleepThread() {
    pthread_t tid = 0;
    pthread_create(&tid, NULL, sleep_thread_func, NULL);
    pthread_detach(tid);
    return 0;
}

AX_BOOL GetLocalIP(AX_CHAR* pOutIPAddress, AX_U32 nLen)
{
    const char* vNetType[] = {"eth", "usb"};
    for (size_t i = 0; i < 2; i++) {
        for (char c = '0'; c <= '9'; ++c) {
            AX_CHAR pszDevice[64] = {0};
            sprintf(pszDevice, "%s%c", vNetType[i], c);
            int fd;
            int ret;
            struct ifreq ifr;
            fd = socket(AF_INET, SOCK_DGRAM, 0);
            strcpy(ifr.ifr_name, pszDevice);
            ret = ioctl(fd, SIOCGIFADDR, &ifr);
            close(fd);
            if (ret < 0) {
                continue;
            }

            char* pIP = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
            if (pIP) {
                strncpy((char *)pOutIPAddress, pIP, nLen - 1);
                pOutIPAddress[nLen - 1] = '\0';
                return AX_TRUE;
            }
        }
    }
    return AX_FALSE;
}

// read and update running times
AX_S32 GetRunningTimes(AX_VOID) {
    FILE* fStream;
    AX_S32 nTimes = 0;
    AX_S32 nRetry = 30;
    const char *path = GetFlagFilePath(E_FLAG_FILE_TIMES);
    if (!QS_CheckMounted(path, nRetry)) {
        LogToFile("[GRT mounted check] failed");
        return nTimes;
    }

    fStream = fopen(GetFlagFile(E_FLAG_FILE_TIMES), "r+");
    if (fStream) {
        fread(&nTimes, 1, sizeof(nTimes), fStream);
    } else {
        LogToFile("[GRT fopen r+ Error] %d : %s.", errno, strerror(errno));
        fStream = fopen(GetFlagFile(E_FLAG_FILE_TIMES), "w");
    }

    if (fStream) {
        ++nTimes;
        if (0 != fseek(fStream, 0, SEEK_SET)) {
            LogToFile("[GRT fseek Error] %d : %s.", errno, strerror(errno));
        }

        // write new value
        if (sizeof(nTimes) != fwrite(&nTimes, 1, sizeof(nTimes), fStream)) {
            LogToFile("[GRT fwrite Error] %d : %s.", errno, strerror(errno));
        }

        if (0 != fflush(fStream)) {
            LogToFile("[GRT fflush Error] %d : %s.", errno, strerror(errno));
        }

        if (0 != fclose(fStream)) {
            LogToFile("[GRT fclose Error] %d : %s.", errno, strerror(errno));
        }
    }
    else {
        LogToFile("[GRT fopen w Error] %d : %s.", errno, strerror(errno));
    }

    return nTimes;
}

void UpdateRunTime(AX_U64* pRunTime, AX_U64* pLastTime) {
    AX_U64 u64CurPts = GetTickCountPts();
    *pRunTime = u64CurPts - (*pLastTime);
    *pLastTime = u64CurPts;
}

AX_VOID PrintLaunchTimeResult()
{
    AX_U64 u64Started[2] = {0};
    AX_U64 u64Opended[2] = {0};
    AX_U64 u64FrameReady[2] = {0};
    AX_U64 u64OutToNext[2] = {0};
    AX_U64 u64Fsof[2] = {0};

    if (!g_bNeedShowLauchResult) {
        return;
    }

    if (g_entry_param.nCamCnt == 1) {
        if ((g_entry_param.bDetectEnable && (g_stResultTimes.u64DetectTimeFromBoot[0] == 0)) ||
            (g_entry_param.bVencEnable && (g_stResultTimes.u64Venc0TimeFromBoot[0] == 0))) {
            return;
        }
    } else {
        if ((g_entry_param.bDetectEnable && (g_stResultTimes.u64DetectTimeFromBoot[0] == 0 || g_stResultTimes.u64DetectTimeFromBoot[1] == 0)) ||
            (g_entry_param.bVencEnable && (g_stResultTimes.u64Venc0TimeFromBoot[0] == 0 || g_stResultTimes.u64Venc0TimeFromBoot[1] == 0))) {
            return;
        }
    }

    g_bNeedShowLauchResult = AX_FALSE;
    g_nRunningTimes = GetRunningTimes();
    ReadISPTimes(&u64Started[0], &u64Opended[0], &u64FrameReady[0], &u64OutToNext[0], &u64Fsof[0], g_entry_param.nCamCnt);

    LogToFile("main launch timestamp:  %llu", g_u64LaunchPts);
    if (g_entry_param.nCamCnt == 1) {
        LogToFile("round[%d] latency result: 1st Fsof: %llu, 1st Raw Image: %llu, 1st Detect Result: %llu, 1st Venc Frame: %llu, main launch: %llu.",
                g_nRunningTimes,
                u64Fsof[0],
                u64FrameReady[0],
                g_stResultTimes.u64DetectTimeFromBoot[0],
                g_stResultTimes.u64Venc0TimeFromBoot[0],
                g_u64LaunchPts);

        ALOGI("round[%d] yuv latency from boot: %llu.", g_nRunningTimes,  g_stResultTimes.u64DetectYUVTimeFromBoot[0]);
    } else {
        LogToFile("round[%d] latency result: 1st Fsof: [%llu, %llu], 1st Raw Image: [%llu, %llu], 1st Detect Result: [%llu, %llu], 1st Venc Frame: [%llu, %llu] main launch: %llu.",
                g_nRunningTimes,
                u64Fsof[0],
                u64Fsof[1],
                u64FrameReady[0],
                u64FrameReady[1],
                g_stResultTimes.u64DetectTimeFromBoot[0],
                g_stResultTimes.u64DetectTimeFromBoot[1],
                g_stResultTimes.u64Venc0TimeFromBoot[0],
                g_stResultTimes.u64Venc0TimeFromBoot[1],
                g_u64LaunchPts);

        ALOGI("round[%d] yuv latency from boot: [%llu, %llu].", g_nRunningTimes,  g_stResultTimes.u64DetectYUVTimeFromBoot[0], g_stResultTimes.u64DetectYUVTimeFromBoot[1]);
    }

    LogToFile("round[%d] vin latency sns[0]: started: %llu, opened: %llu, 1st raw ready: %llu, out next: %llu, 1st fsof: %llu",
              g_nRunningTimes,
              u64Started[0],
              u64Opended[0],
              u64FrameReady[0],
              u64OutToNext[0],
              u64Fsof[0]);

    if (g_entry_param.nCamCnt == 2) {
        LogToFile("round[%d] vin latency sns[1]: started: %llu, opened: %llu, 1st raw ready: %llu, out next: %llu, 1st fsof: %llu",
              g_nRunningTimes,
              u64Started[1],
              u64Opended[1],
              u64FrameReady[1],
              u64OutToNext[1],
              u64Fsof[1]);
    }
}

static AX_BOOL CheckRebootCondition(AX_U64 u64GetVencFrmCnt) {
    AX_U64 u64CurPts = GetTickCountPts();
    if (g_entry_param.nRebootInterval > 0) {
        if ((u64CurPts - g_u64LaunchPts) >= g_entry_param.nRebootInterval*1000000) {
            if (g_NeedRebootSystem && CheckStopFlag()) {
                g_NeedRebootSystem = AX_FALSE;
            } else if (g_nQSType == QSReboot && !CheckStopFlag()) {
                g_NeedRebootSystem = AX_TRUE;
            }
            return AX_TRUE;
        }
    } else {
        if (u64GetVencFrmCnt >= g_entry_param.nVencDumpNum || g_NeedRebootSystem) {
            return AX_TRUE;
        }
    }
    return AX_FALSE;
}

/* venc get stream task */
static void *VencGetStreamProc(void *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_RECV_PIC_PARAM_T stRecvParam;
    VENC_GETSTREAM_PARAM_T *pstPara = (VENC_GETSTREAM_PARAM_T *)arg;
    AX_VENC_STREAM_T stStream = {0};
    FILE *pStrm = NULL;
    AX_U64 u64LastPts = 0;
    AX_U64 u64CurPts = 0;
    AX_U64 u64VencFileSize = 0;
    AX_BOOL bClosed = AX_FALSE;
    pstPara->u64GetVencFrmCnt = 0;
    pstPara->u64LastFramePts = 0;
    AX_U32 nRcvFrmFromWakeup = 0;
    AX_BOOL bVencAllFrmGotFromWakeup = AX_FALSE;

    AX_CHAR szName[50] = {0};
    sprintf(szName, "qs_venc_%d", pstPara->nVeChn);
    prctl(PR_SET_NAME, szName);

    s32Ret = AX_VENC_StartRecvFrame(pstPara->nVeChn, &stRecvParam);
    if (AX_SUCCESS != s32Ret) {
        ALOGE("Venc[%d]: AX_VENC_StartRecvFrame failed, s32Ret:0x%x", pstPara->nVeChn, s32Ret);
        return NULL;
    }

    AX_BOOL bChangedToSD = AX_FALSE;

    AX_CHAR szNewPath[128] = {0};
    strcpy(szNewPath, gH264Path[pstPara->nSndId]);
    AX_S32 nWritenLen = 0;

    while (pstPara->bThreadStart && !ThreadLoopStateGet()) {
        s32Ret = AX_VENC_GetStream(pstPara->nVeChn, &stStream, 500);
        if (AX_SUCCESS == s32Ret) {
            if (g_nQSType == QSAOV && g_nLastFps == 1 && (pstPara->nVeChn % IVPSChannelNumber) == 0) {
                // for debug: timestamp get venc
                AX_SYS_SleepTimeStamp(AX_ID_USER, pstPara->nVeChn == 0 ? E_STS_GET_VENC_0 : E_STS_GET_VENC_1);
            }

            AX_BOOL bIFrame = (AX_VENC_INTRA_FRAME == stStream.stPack.enCodingType || PT_MJPEG == stStream.stPack.enType) ? AX_TRUE : AX_FALSE;
            u64CurPts = GetTickCountPts();

            if ((pstPara->u64LastSeqNum > 0) &&  ((stStream.stPack.u64SeqNum - pstPara->u64LastSeqNum) != 1)) {
                // 15fps, 66ms between frames, over range (61, 71) is lost
                if (stStream.stPack.u64PTS <= u64LastPts || (stStream.stPack.u64PTS - u64LastPts) > 71000 ||
                    (stStream.stPack.u64PTS - u64LastPts) < 61000) {
                    if (IS_IGNORE(pstPara->nSndId, pstPara->nVeChn)) {
                        RESET_IGNORE(pstPara->nSndId, pstPara->nVeChn);
                    } else {
                        LogToFile("Venc[%d]: Frame was lost, seqno: %llu -> %llu, pts: %llu -> %llu",
                        pstPara->nVeChn, pstPara->u64LastSeqNum, stStream.stPack.u64SeqNum, u64LastPts, stStream.stPack.u64PTS);
                    }
                }
            }

            pthread_mutex_lock(&g_mutex);

            if (g_nQSType == QSAOV && g_nLastFps == 1) {
                g_bSleepFlag = AX_FALSE;
            }

            if (g_nQSType == QSAOV && g_nLastFps == 1 && (pstPara->nVeChn % IVPSChannelNumber) == 0) {
                ++nRcvFrmFromWakeup;
                ALOGI("Venc[%d]: got packet [%d] seqno=%llu", pstPara->nVeChn, nRcvFrmFromWakeup, stStream.stPack.u64SeqNum);
                if (nRcvFrmFromWakeup >= g_entry_param.nCapNum) {
                    nRcvFrmFromWakeup = 0;
                    bVencAllFrmGotFromWakeup = AX_TRUE;
                }
            }

            pstPara->u64LastSeqNum = stStream.stPack.u64SeqNum;
            u64LastPts = stStream.stPack.u64PTS;

            pthread_mutex_unlock(&g_mutex);

            if (g_bNeedShowLauchResult && (pstPara->nVeChn % IVPSChannelNumber) == 0 && pstPara->u64GetVencFrmCnt == 0) {
                g_stResultTimes.u64Venc0TimeFromBoot[pstPara->nSndId] = u64CurPts;
                PrintLaunchTimeResult();
                LogToFile("Venc[%d]: 1st packet seqno: %llu.", pstPara->nVeChn, stStream.stPack.u64SeqNum);
            }

            if ((pstPara->nVeChn % IVPSChannelNumber) == 0 && g_entry_param.nVencDumpNum > 0 && pStrm == NULL) {
                pStrm = fopen(gH264Path[pstPara->nSndId], "wb");
            }

            if (!bClosed && pStrm != NULL && QS_IsSDCardReady() && !bChangedToSD && g_nQSType != QSAOV && g_entry_param.nRebootInterval > 0) {

                fflush(pStrm);
                fclose(pStrm);
                AX_CHAR szCmd[256] = {0};
                QS_GetSDRuntimePath(szNewPath, 128, g_nRunningTimes, AX_TRUE);
                AX_S32 nCount = sprintf(szCmd, "mv -f %s %s/venc%d.264", gH264Path[pstPara->nSndId], szNewPath, pstPara->nSndId);
                szCmd[nCount] = 0;
                QS_RunCmd(szCmd, NULL, 0);

                nCount = sprintf(szCmd, "%s/venc%d.264", szNewPath, pstPara->nSndId);
                szCmd[nCount] = 0;
                pStrm = fopen(szCmd, "ab");
                bChangedToSD = AX_TRUE;
                ALOGI("Venc[%d]: seq=%llu, change store path: %s", pstPara->nVeChn, stStream.stPack.u64SeqNum, szCmd);
            }

            ++pstPara->u64GetVencFrmCnt;
            pstPara->u64LastFramePts = u64CurPts;

            if (g_nQSType == QSAOV && (pstPara->nVeChn % IVPSChannelNumber) == 0) {
                if (stStream.stPack.pu8Addr && stStream.stPack.u32Len > 0) {
                    QS_SaveVideo(pstPara->nSndId, stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.u64PTS, bIFrame, AX_FALSE);
                    if (bVencAllFrmGotFromWakeup) {
                        pthread_mutex_lock(&g_mutex);
                        ALOGI("Venc[%d]: packet seqno=%llu is cached", pstPara->nVeChn, stStream.stPack.u64SeqNum);
                        if (pstPara->nSndId == 0) {
                            update_sleep_condition(CONDITION_MASK_VENC0, true);
                        } else if (pstPara->nSndId == 1) {
                            update_sleep_condition(CONDITION_MASK_VENC1, true);
                        }

                        pthread_mutex_unlock(&g_mutex);
                        bVencAllFrmGotFromWakeup = AX_FALSE;
                    }
                }
            }

            if(!bClosed && pStrm != NULL && !CheckRebootCondition(pstPara->u64GetVencFrmCnt)) {
                if (stStream.stPack.pu8Addr && stStream.stPack.u32Len > 0) {
                    u64VencFileSize += stStream.stPack.u32Len;
                    nWritenLen = fwrite(stStream.stPack.pu8Addr, 1, stStream.stPack.u32Len, pStrm);
                    if (nWritenLen != stStream.stPack.u32Len) {
                        if (bChangedToSD && !QS_IsSDCardReady()) {
                            fclose(pStrm);
                            bClosed = AX_TRUE;
                        }
                    }
                }
            }

            /* Send to RTSP */
            if (stStream.stPack.pu8Addr && stStream.stPack.u32Len > 0) {
                if (g_hRtsp) {
                    AX_Rtsp_SendVideo(g_hRtsp, gOutChnAttr[pstPara->nVeChn].nRtspChn, stStream.stPack.pu8Addr, stStream.stPack.u32Len, stStream.stPack.u64PTS, bIFrame);
                }
            }

            s32Ret = AX_VENC_ReleaseStream(pstPara->nVeChn, &stStream);

            // for debug
            // if (!g_entry_param.bDetectEnable && (pstPara->nVeChn % IVPSChannelNumber) == 0  && totalGetStream == g_entry_param.nDetectFrameMax) {
            //     NoBodyFoundNotify();
            // }

            if (pStrm && CheckRebootCondition(pstPara->u64GetVencFrmCnt)) {
                if (!bClosed) {
                    fflush(pStrm);
                    ALOGW("Venc[%d]: H264 is saved: %s, size: %llu bytes", pstPara->nVeChn, szNewPath, u64VencFileSize);
                    fclose(pStrm);
                    bClosed = AX_TRUE;
                    if (g_nQSType == QSAOV) {
                        // delete tmp video file
                        if (QS_IsSDCardReady()) {
                            unlink(gH264Path[pstPara->nSndId]);
                            ALOGI("delete file %s to free tmp folder", gH264Path[pstPara->nSndId]);
                        }
                    }
                }

                if (g_NeedRebootSystem) {
                    ThreadLoopStateSet(AX_TRUE);
                }
            }

            if (AX_SUCCESS != s32Ret) {
                ALOGE("Venc[%d]: AX_VENC_ReleaseStream failed, error: 0x%x", pstPara->nVeChn, s32Ret);
                goto EXIT;
            }

        } else if (AX_ERR_VENC_FLOW_END == s32Ret) {
            goto EXIT;
        } else if (AX_ERR_VENC_OUTBUF_OVERFLOW == s32Ret) {
            ALOGE("Venc[%d]: AX_VENC_GetStream got overflow", pstPara->nVeChn);
            pthread_mutex_lock(&g_mutex);
            if (g_nQSType == QSAOV && g_nLastFps == 1) {
                g_bSleepFlag = AX_FALSE;
            }

            if (g_nQSType == QSAOV && g_nLastFps == 1 && (pstPara->nVeChn % IVPSChannelNumber) == 0) {
                ++nRcvFrmFromWakeup;
                ALOGE("Venc[%d]: packet [%d] overflow", pstPara->nVeChn, nRcvFrmFromWakeup);
                if (nRcvFrmFromWakeup >= g_entry_param.nCapNum) {
                    nRcvFrmFromWakeup = 0;
                    bVencAllFrmGotFromWakeup = AX_TRUE;
                }
            }

            AX_SYS_GetCurPTS(&u64LastPts);

            pthread_mutex_unlock(&g_mutex);

            if (g_nQSType == QSAOV && (pstPara->nVeChn % IVPSChannelNumber) == 0) {
                if (bVencAllFrmGotFromWakeup) {
                    pthread_mutex_lock(&g_mutex);
                    if (pstPara->nSndId == 0) {
                        update_sleep_condition(CONDITION_MASK_VENC0, true);
                    } else if (pstPara->nSndId == 1) {
                        update_sleep_condition(CONDITION_MASK_VENC1, true);
                    }

                    pthread_mutex_unlock(&g_mutex);
                    bVencAllFrmGotFromWakeup = AX_FALSE;
                }
            }
        }
    }

EXIT:
    if (!bClosed && pStrm != NULL) {
        fclose(pStrm);
        pStrm = NULL;
    }
    return NULL;
}

/* jenc get stream task */
static void *JencGetStreamProc(void *arg)
{
    AX_S32 s32Ret = -1;
    AX_VENC_RECV_PIC_PARAM_T stRecvParam;
    VENC_GETSTREAM_PARAM_T *pstPara = (VENC_GETSTREAM_PARAM_T *)arg;
    AX_VENC_STREAM_T stStream = {0};
    AX_U32 totalGetStream = 0;

    AX_CHAR szName[50] = {0};
    sprintf(szName, "qs_jenc_%d", pstPara->nVeChn);
    prctl(PR_SET_NAME, szName);

    s32Ret = AX_VENC_StartRecvFrame(pstPara->nVeChn, &stRecvParam);
    if (AX_SUCCESS != s32Ret) {
        ALOGE("Jenc[%d]: AX_VENC_StartRecvFrame failed, error: 0x%x", pstPara->nVeChn, s32Ret);
        return NULL;
    }

    while (pstPara->bThreadStart && !ThreadLoopStateGet()) {
        s32Ret = AX_VENC_GetStream(pstPara->nVeChn, &stStream, 200);
        if (AX_SUCCESS == s32Ret) {
            if (totalGetStream == 0 && g_entry_param.bJencDump) {
                FILE *pStrm = fopen(gJpegPath[pstPara->nSndId], "wb");
                if(pStrm != NULL) {
                    fwrite(stStream.stPack.pu8Addr, 1, stStream.stPack.u32Len, pStrm);
                    fflush(pStrm);
                    fclose(pStrm);
                    pStrm = NULL;
                    ALOGW("Jenc[%d]: 1st image is saved: %s, seqno: %llu, pts: %llu, size: %u",
                          pstPara->nVeChn, gJpegPath[pstPara->nSndId], stStream.stPack.u64SeqNum, stStream.stPack.u64PTS, stStream.stPack.u32Len);
                }
            }
            ++totalGetStream;
            s32Ret = AX_VENC_ReleaseStream(pstPara->nVeChn, &stStream);
            if (AX_SUCCESS != s32Ret) {
                ALOGE("Jenc[%d]: AX_VENC_ReleaseStream failed, error: 0x%x", pstPara->nVeChn, s32Ret);
                break;
            }
        } else if (AX_ERR_VENC_FLOW_END == s32Ret) {
            break;
        }
    }

    return NULL;
}

/* 2M: 2M; 4M: H264 4M, H265: 2M; 4K: H264: 8M, H265:4M */
static AX_S32 GetVencBitrate(AX_PAYLOAD_TYPE_E enType, AX_S32 nWidth, AX_S32 nHeight) {
    if (PT_H264 == enType) {
        // 4K
        if (nWidth >= 3840) {
            return 8192;
        }
        // 4M
        else if (nWidth >= 2560) {
            return 4096;
        }
        // 2M
        else if (nWidth >= 1920) {
            return 1280;
        }
        else {
            return ALIGN_COMM_DOWN(nWidth * nHeight / 1000, 100);
        }
    } else if (PT_H265 == enType) {
        // 4K
        if (nWidth >= 3840) {
            return 4096;
        }
        // 4M
        else if (nWidth >= 2560) {
            return 2048;
        }
        // 2M
        else if (nWidth >= 1920) {
            return 1024;
        }
        else {
            return ALIGN_COMM_DOWN(nWidth * nHeight / 1000, 100);
        }
    }

    return 2048;
}

static AX_S32 SampleVencInit(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    AX_VENC_CHN_ATTR_T stVencChnAttr;
    VIDEO_CONFIG_T config = {0};
    AX_S32 nVencChn = 0, s32Ret = 0;
    AX_VENC_MOD_ATTR_T stModAttr;

    memset(&stModAttr, 0x0, sizeof(stModAttr));
    stModAttr.enVencType = AX_VENC_MULTI_ENCODER;
    stModAttr.stModThdAttr.u32TotalThreadNum = 3;
    stModAttr.stModThdAttr.bExplicitSched = AX_FALSE;

    s32Ret = AX_VENC_Init(&stModAttr);
    if (AX_SUCCESS != s32Ret) {
        ALOGE("AX_VENC_Init failed, error: 0x%x", s32Ret);
        return s32Ret;
    }

    if (!pEntryParam->bVencEnable) {
        return 0;
    }

    for (nVencChn = 0; nVencChn < pEntryParam->nIvpsChnCnt*pEntryParam->nCamCnt; nVencChn++) {
        if (!gOutChnAttr[nVencChn].bVenc) {
            continue;
        }
        config.nInWidth = gOutChnAttr[nVencChn].nWidth;
        config.nInHeight = gOutChnAttr[nVencChn].nHeight;
        config.nStride = AX_ALIGN_UP(gOutChnAttr[nVencChn].nWidth, 128);

        config.nGOP = pEntryParam->nSensorFrameRate * 2;
        if (config.nInWidth == 1080) {
            config.stRCInfo.eRCType = SAMPLE_RC_VBR;
            config.stRCInfo.nMinQp = 31;
            config.stRCInfo.nMaxQp = 48;
            config.stRCInfo.nMinIQp = 31;
            config.stRCInfo.nMaxIQp = 48;
        }
        else {
            config.stRCInfo.eRCType = SAMPLE_RC_CBR;
            config.stRCInfo.nMinQp = 10;
            config.stRCInfo.nMaxQp = 51;
            config.stRCInfo.nMinIQp = 10;
            config.stRCInfo.nMaxIQp = 51;
        }
        config.stRCInfo.nIntraQpDelta = -2;

        if (pEntryParam->nGdcOnlineVppTest) {
            AX_S32 nTmp = config.nInWidth;
            config.nInWidth = config.nInHeight;
            config.nInHeight = nTmp;
            config.nStride = AX_ALIGN_UP(config.nInWidth, 128);
        }

        config.ePayloadType = PT_H264;
        config.nSrcFrameRate = pEntryParam->nSensorFrameRate;
        config.nDstFrameRate = pEntryParam->nSensorFrameRate;
        config.nBitrate = GetVencBitrate(config.ePayloadType, config.nInWidth, config.nInHeight);

        memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

        stVencChnAttr.stVencAttr.u32MaxPicWidth = config.nInWidth;
        stVencChnAttr.stVencAttr.u32MaxPicHeight = config.nInHeight;
#if defined(SAMPLE_SNS_SC200AI_TRIPLE)
        stVencChnAttr.stVencAttr.u8InFifoDepth = 4;
        stVencChnAttr.stVencAttr.u8OutFifoDepth = 4;
#else
        stVencChnAttr.stVencAttr.u8InFifoDepth = 1;
        stVencChnAttr.stVencAttr.u8OutFifoDepth = 1;
#endif
        stVencChnAttr.stVencAttr.u32PicWidthSrc = config.nInWidth;
        stVencChnAttr.stVencAttr.u32PicHeightSrc = config.nInHeight;
        stVencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate  = config.nSrcFrameRate;
        stVencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = config.nDstFrameRate;

        if (config.nInWidth > 1920) {
            // resolution over 2M
            stVencChnAttr.stVencAttr.u32BufSize = config.nInWidth * config.nInHeight * 3 / 4;
            if (stVencChnAttr.stVencAttr.u32BufSize < 3*1024*1024) {
                stVencChnAttr.stVencAttr.u32BufSize = 3*1024*1024;
            }
        } else {
            // resolution equal or below 2M
            stVencChnAttr.stVencAttr.u32BufSize = config.nInWidth * config.nInHeight * 3 / 4;
        }

        if (stVencChnAttr.stVencAttr.u32BufSize < 512*1024) {
            // the min size is 512K
            stVencChnAttr.stVencAttr.u32BufSize = 512*1024;
        }

        stVencChnAttr.stVencAttr.enLinkMode = AX_LINK_MODE;
        stVencChnAttr.stVencAttr.enMemSource = AX_MEMORY_SOURCE_CMM;

        stVencChnAttr.stVencAttr.bDeBreathEffect = AX_FALSE;
        stVencChnAttr.stVencAttr.bRefRingbuf = AX_TRUE;

        /* GOP Setting */
        stVencChnAttr.stGopAttr.enGopMode = AX_VENC_GOPMODE_NORMALP;

        stVencChnAttr.stVencAttr.enType = config.ePayloadType;

        switch (stVencChnAttr.stVencAttr.enType) {
            case PT_H265: {
                stVencChnAttr.stVencAttr.enProfile = AX_VENC_HEVC_MAIN_PROFILE;
                stVencChnAttr.stVencAttr.enLevel = AX_VENC_HEVC_LEVEL_5_1;
                stVencChnAttr.stVencAttr.enTier = AX_VENC_HEVC_MAIN_TIER;

                if (config.stRCInfo.eRCType == SAMPLE_RC_CBR) {
                    AX_VENC_H265_CBR_T stH265Cbr;
                    memset(&stH265Cbr, 0, sizeof(AX_VENC_H265_CBR_T));
                    stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265CBR;
                    stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                    stH265Cbr.u32Gop = config.nGOP;
                    stH265Cbr.u32BitRate = config.nBitrate;
                    stH265Cbr.u32MinQp = config.stRCInfo.nMinQp;
                    stH265Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
                    stH265Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
                    stH265Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                    stH265Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                    stH265Cbr.u32MinIprop = 30;
                    stH265Cbr.u32MaxIprop = 40;
                    memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(AX_VENC_H265_CBR_T));
                } else if (config.stRCInfo.eRCType == SAMPLE_RC_VBR) {
                    AX_VENC_H265_VBR_T stH265Vbr;
                    memset(&stH265Vbr, 0, sizeof(AX_VENC_H265_VBR_T));
                    stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265VBR;
                    stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                    stH265Vbr.u32Gop = config.nGOP;
                    stH265Vbr.u32MaxBitRate = config.nBitrate;
                    stH265Vbr.u32MinQp = config.stRCInfo.nMinQp;
                    stH265Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
                    stH265Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
                    stH265Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                    stH265Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                    memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(AX_VENC_H265_VBR_T));
                } else if (config.stRCInfo.eRCType == SAMPLE_RC_FIXQP) {
                    AX_VENC_H265_FIXQP_T stH265FixQp;
                    memset(&stH265FixQp, 0, sizeof(AX_VENC_H265_FIXQP_T));
                    stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H265FIXQP;
                    stH265FixQp.u32Gop = config.nGOP;
                    stH265FixQp.u32IQp = 25;
                    stH265FixQp.u32PQp = 30;
                    stH265FixQp.u32BQp = 32;
                    memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(AX_VENC_H265_FIXQP_T));
                }
                break;
            }
            case PT_H264: {
                stVencChnAttr.stVencAttr.enProfile = AX_VENC_H264_MAIN_PROFILE;
                stVencChnAttr.stVencAttr.enLevel = AX_VENC_H264_LEVEL_5_2;

                if (config.stRCInfo.eRCType == SAMPLE_RC_CBR) {
                    AX_VENC_H264_CBR_T stH264Cbr;
                    memset(&stH264Cbr, 0, sizeof(AX_VENC_H264_CBR_T));
                    stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264CBR;
                    stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                    stH264Cbr.u32Gop = config.nGOP;
                    stH264Cbr.u32BitRate = config.nBitrate;
                    stH264Cbr.u32MinQp = config.stRCInfo.nMinQp;
                    stH264Cbr.u32MaxQp = config.stRCInfo.nMaxQp;
                    stH264Cbr.u32MinIQp = config.stRCInfo.nMinIQp;
                    stH264Cbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                    stH264Cbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                    stH264Cbr.u32MinIprop = 10;
                    stH264Cbr.u32MaxIprop = 40;
                    memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(AX_VENC_H264_CBR_T));
                } else if (config.stRCInfo.eRCType == SAMPLE_RC_VBR) {
                    AX_VENC_H264_VBR_T stH264Vbr;
                    memset(&stH264Vbr, 0, sizeof(AX_VENC_H264_VBR_T));
                    stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264VBR;
                    stVencChnAttr.stRcAttr.s32FirstFrameStartQp = -1;
                    stH264Vbr.u32Gop = config.nGOP;
                    stH264Vbr.u32MaxBitRate = config.nBitrate;
                    stH264Vbr.u32MinQp = config.stRCInfo.nMinQp;
                    stH264Vbr.u32MaxQp = config.stRCInfo.nMaxQp;
                    stH264Vbr.u32MinIQp = config.stRCInfo.nMinIQp;
                    stH264Vbr.u32MaxIQp = config.stRCInfo.nMaxIQp;
                    stH264Vbr.s32IntraQpDelta = config.stRCInfo.nIntraQpDelta;
                    memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(AX_VENC_H264_VBR_T));
                } else if (config.stRCInfo.eRCType == SAMPLE_RC_FIXQP) {
                    AX_VENC_H264_FIXQP_T stH264FixQp;
                    memset(&stH264FixQp, 0, sizeof(AX_VENC_H264_FIXQP_T));
                    stVencChnAttr.stRcAttr.enRcMode = AX_VENC_RC_MODE_H264FIXQP;
                    stH264FixQp.u32Gop = config.nGOP;
                    stH264FixQp.u32IQp = 25;
                    stH264FixQp.u32PQp = 30;
                    stH264FixQp.u32BQp = 32;
                    memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(AX_VENC_H264_FIXQP_T));
                }
                break;
            }
            default:
                ALOGE("Venc[%d]: Payload type unrecognized.",nVencChn);
                return -1;
        }

        AX_S32 ret = AX_VENC_CreateChn(nVencChn, &stVencChnAttr);
        if (AX_SUCCESS != ret) {
            ALOGE("Venc[%d]: AX_VENC_CreateChn failed, error: 0x%x", nVencChn, ret);
            return -1;
        }

        /* create get output stream thread */
        gGetStreamParam[nVencChn].nVeChn = nVencChn;
        gGetStreamParam[nVencChn].bThreadStart = AX_TRUE;
        gGetStreamParam[nVencChn].ePayloadType = config.ePayloadType;
        gGetStreamParam[nVencChn].nSndId = nVencChn / pEntryParam->nIvpsChnCnt;
        pthread_create(&gGetStreamParam[nVencChn].nTid, NULL, VencGetStreamProc, (void *)&gGetStreamParam[nVencChn]);
    }

    return 0;
}

static AX_S32 SampleJencInit(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    AX_VENC_CHN_ATTR_T stVencChnAttr;
    AX_S32 nJencChn = 0, s32Ret = 0;
    AX_VENC_JPEG_PARAM_T stJpegParam;

    for (nJencChn = 0; nJencChn < pEntryParam->nIvpsChnCnt*pEntryParam->nCamCnt; nJencChn++) {
        if (gOutChnAttr[nJencChn].bVenc) {
            continue;
        }

        AX_S32 nInWidth = gOutChnAttr[nJencChn].nWidth;
        AX_S32 nInHeight = gOutChnAttr[nJencChn].nHeight;

        if (pEntryParam->nGdcOnlineVppTest) {
            AX_S32 nTmp = nInWidth;
            nInWidth = nInHeight;
            nInHeight = nTmp;
        }

        memset(&stVencChnAttr, 0, sizeof(stVencChnAttr));
        stVencChnAttr.stVencAttr.enType = PT_JPEG;
        stVencChnAttr.stVencAttr.u32MaxPicHeight = MAX_HEIGHT_DEFAULT;
        stVencChnAttr.stVencAttr.u32MaxPicWidth = MAX_WIDTH_DEFAULT;
        stVencChnAttr.stVencAttr.enLinkMode = AX_LINK_MODE;
        stVencChnAttr.stVencAttr.u8InFifoDepth = 1;
        stVencChnAttr.stVencAttr.u8OutFifoDepth = 1;
        stVencChnAttr.stVencAttr.u32PicWidthSrc = gOutChnAttr[nJencChn].nWidth;
        stVencChnAttr.stVencAttr.u32PicHeightSrc = gOutChnAttr[nJencChn].nHeight;
        stVencChnAttr.stVencAttr.u32BufSize = stVencChnAttr.stVencAttr.u32PicWidthSrc * stVencChnAttr.stVencAttr.u32PicHeightSrc * 3 / 4;
        stVencChnAttr.stRcAttr.stFrameRate.fSrcFrameRate = pEntryParam->nSensorFrameRate;
        stVencChnAttr.stRcAttr.stFrameRate.fDstFrameRate = pEntryParam->nJencFrameRate;

        s32Ret = AX_VENC_CreateChn(nJencChn, &stVencChnAttr);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("Jenc[%d]: AX_VENC_CreateChn failed, error: 0x%x", nJencChn, s32Ret);
            return -1;
        }

        memset(&stJpegParam, 0, sizeof(stJpegParam));
        s32Ret = AX_VENC_GetJpegParam(nJencChn, &stJpegParam);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("Jenc[%d]: AX_VENC_GetJpegParam failed, error: 0x%x", nJencChn, s32Ret);
            AX_VENC_DestroyChn(nJencChn);
            return -1;
        }

        stJpegParam.u32Qfactor = 60;
        s32Ret = AX_VENC_SetJpegParam(nJencChn, &stJpegParam);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("Jenc[%d]: AX_VENC_SetJpegParam failed, error: 0x%x", nJencChn, s32Ret);
            AX_VENC_StopRecvFrame(nJencChn);
            AX_VENC_DestroyChn(nJencChn);
            return -1;
        }

        /* create get output stream thread */
        gGetStreamParam[nJencChn].nVeChn = nJencChn;
        gGetStreamParam[nJencChn].nSndId = nJencChn / pEntryParam->nIvpsChnCnt;
        gGetStreamParam[nJencChn].bThreadStart = AX_TRUE;
        pthread_create(&gGetStreamParam[nJencChn].nTid, NULL, JencGetStreamProc, (void *)&gGetStreamParam[nJencChn]);
    }

    return 0;
}

static AX_S32 SampleVencDestroy(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    ALOGI("SampleVencDestroy ++");
    AX_S32 VencChn = 0, s32Ret = 0;
    AX_S32 nRetry = 5;

    for (VencChn = 0; VencChn < pEntryParam->nIvpsChnCnt * pEntryParam->nCamCnt; VencChn++) {
        if (!pEntryParam->bVencEnable && gOutChnAttr[VencChn].bVenc) {
            continue;
        }

        if ((VencChn % pEntryParam->nIvpsChnCnt) == JENC_OUT_CHN && !pEntryParam->bJencEnable) {
            continue;
        }

        s32Ret = AX_VENC_StopRecvFrame(VencChn);
        if (0 != s32Ret) {
            ALOGE("Venc[%d]: AX_VENC_StopRecvFrame failed, error: 0x%x", VencChn, s32Ret);
        }

        do {
            s32Ret = AX_VENC_DestroyChn(VencChn);
            if (AX_ERR_VENC_BUSY == s32Ret) {
                MSSleep(100);
                --nRetry;
            } else {
                break;
            }
        } while(nRetry >= 0);

        if (nRetry == -1) {
            ALOGE("Venc[%d]: AX_VENC_DestroyChn retry 5 times failed", VencChn);
        }

        if (AX_TRUE == gGetStreamParam[VencChn].bThreadStart) {
            gGetStreamParam[VencChn].bThreadStart = AX_FALSE;
            pthread_join(gGetStreamParam[VencChn].nTid, NULL);
        }
    }

    ALOGI("SampleVencDestroy --");
    return s32Ret;
}

static AX_S32 SampleVencDeInit(AX_VOID)
{
    ALOGI("SampleVencDeInit ++");
    AX_S32 s32Ret = 0;

    s32Ret = AX_VENC_Deinit();
    if (AX_SUCCESS != s32Ret) {
        ALOGE("AX_VENC_Deinit failed, error: 0x%x", s32Ret);
    }

    ALOGI("SampleVencDeInit --");
    return s32Ret;
}

static int SampleIvpsInit(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    AX_S32 s32Ret = 0, nChn = 0;

    AX_IVPS_GRP_ATTR_T stGrpAttr = {0};
    AX_IVPS_PIPELINE_ATTR_T stPipelineAttr = {0};

    for(AX_S32 nGrpId = 0; nGrpId < pEntryParam->nCamCnt; nGrpId++) {

        stGrpAttr.nInFifoDepth = 1;
        stGrpAttr.ePipeline = AX_IVPS_PIPELINE_DEFAULT;
        s32Ret = AX_IVPS_CreateGrp(nGrpId, &stGrpAttr);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("AX_IVPS_CreateGrp failed, grp %d, error: 0x%x", nGrpId, s32Ret);
            return s32Ret;
        }

        AX_S32 nChnIdx = nGrpId*pEntryParam->nIvpsChnCnt;
        AX_S32 nWidth = gOutChnAttr[nChnIdx].nWidth;

        stPipelineAttr.nOutChnNum = pEntryParam->nIvpsChnCnt;
        stPipelineAttr.tFilter[0][0].bEngage = AX_TRUE;
        stPipelineAttr.tFilter[0][0].nDstPicWidth = nWidth;
        stPipelineAttr.tFilter[0][0].nDstPicHeight = gOutChnAttr[0].nHeight;
        stPipelineAttr.tFilter[0][0].nDstPicStride =  gOutChnAttr[nChnIdx].enCompressMode ? AX_ALIGN_UP(nWidth, 128) : AX_ALIGN_UP(nWidth, 16);
        stPipelineAttr.tFilter[0][0].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        stPipelineAttr.tFilter[0][0].eEngine = AX_IVPS_ENGINE_VPP;

        for (nChn = 0; nChn < pEntryParam->nIvpsChnCnt; nChn++) {
            nChnIdx = nChn + nGrpId*pEntryParam->nIvpsChnCnt;
            nWidth = gOutChnAttr[nChnIdx].nWidth;
            stPipelineAttr.tFilter[nChn + 1][0].bEngage = AX_TRUE;
            stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth = gOutChnAttr[nChnIdx].nWidth;
            stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight = gOutChnAttr[nChnIdx].nHeight;
            stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride = gOutChnAttr[nChnIdx].enCompressMode ? AX_ALIGN_UP(nWidth, 128) : AX_ALIGN_UP(nWidth, 16);
            stPipelineAttr.tFilter[nChn + 1][0].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
            stPipelineAttr.tFilter[nChn + 1][0].eEngine = AX_IVPS_ENGINE_SCL;
            stPipelineAttr.tFilter[nChn + 1][0].eSclType = (nChn == 0) ? AX_IVPS_SCL_TYPE_PHASE_FILTER : AX_IVPS_SCL_TYPE_BILINEAR;
            stPipelineAttr.tFilter[nChn + 1][0].tCompressInfo.enCompressMode = gOutChnAttr[nChnIdx].enCompressMode;
            stPipelineAttr.tFilter[nChn + 1][0].tCompressInfo.u32CompressLevel = gOutChnAttr[nChnIdx].enCompressMode == AX_COMPRESS_MODE_NONE ? 0 : 4;
            stPipelineAttr.tFilter[nChn + 1][0].tFRC.fSrcFrameRate = pEntryParam->nSensorFrameRate;
            stPipelineAttr.tFilter[nChn + 1][0].tFRC.fDstFrameRate = pEntryParam->nSensorFrameRate;
            if (gOutChnAttr[nChnIdx].bVenc) {
                stPipelineAttr.tFilter[nChn + 1][1].bEngage = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth = stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight = stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicStride = stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride;
                stPipelineAttr.tFilter[nChn + 1][1].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][1].eEngine = AX_IVPS_ENGINE_TDP;
                stPipelineAttr.tFilter[nChn + 1][1].bInplace = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][1].tTdpCfg.bVoOsd = AX_FALSE;
                stPipelineAttr.tFilter[nChn + 1][1].tCompressInfo = stPipelineAttr.tFilter[nChn + 1][0].tCompressInfo;
            } else {
                stPipelineAttr.tFilter[nChn + 1][1].bEngage = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicWidth = stPipelineAttr.tFilter[nChn + 1][0].nDstPicWidth;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicHeight = stPipelineAttr.tFilter[nChn + 1][0].nDstPicHeight;
                stPipelineAttr.tFilter[nChn + 1][1].nDstPicStride = stPipelineAttr.tFilter[nChn + 1][0].nDstPicStride;
                stPipelineAttr.tFilter[nChn + 1][1].eDstPicFormat = AX_FORMAT_YUV420_SEMIPLANAR;
                stPipelineAttr.tFilter[nChn + 1][1].eEngine = AX_IVPS_ENGINE_TDP;
                stPipelineAttr.tFilter[nChn + 1][1].bInplace = AX_TRUE;
                stPipelineAttr.tFilter[nChn + 1][1].tTdpCfg.bVoOsd = AX_FALSE;
                stPipelineAttr.tFilter[nChn + 1][1].tCompressInfo = stPipelineAttr.tFilter[nChn + 1][0].tCompressInfo;
            }

            if (nChn == ALGO_IVPS_CHN && pEntryParam->bDetectEnable) {
                stPipelineAttr.nOutFifoDepth[nChn] = 1;
            } else {
                stPipelineAttr.nOutFifoDepth[nChn] = 0;
            }
        }

        s32Ret = AX_IVPS_SetPipelineAttr(nGrpId, &stPipelineAttr);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("AX_IVPS_SetPipelineAttr failed, grp %d, error: 0x%x", nGrpId, s32Ret);
            return s32Ret;
        }

        for (nChn = 0; nChn < stPipelineAttr.nOutChnNum; nChn++) {
            s32Ret = AX_IVPS_EnableChn(nGrpId, nChn);
            if (AX_SUCCESS != s32Ret) {
                ALOGE("AX_IVPS_EnableChn failed, grp %d, chn %d, error: 0x%x", nGrpId, nChn, s32Ret);
                return s32Ret;
            }
        }

        s32Ret = AX_IVPS_StartGrp(nGrpId);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("AX_IVPS_StartGrp failed, grp %d, error: 0x%x", nGrpId, s32Ret);
            return s32Ret;
        }
    }

    return 0;
}

static AX_S32 SampleIvpsDestroy(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    ALOGI("SampleIvpsDestroy ++");
    AX_S32 s32Ret = 0, nChn = 0;
    AX_S32 nGrpId = 0;

    for (nGrpId = 0; nGrpId < pEntryParam->nCamCnt; nGrpId++) {
        s32Ret = AX_IVPS_StopGrp(nGrpId);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("AX_IVPS_StopGrp failed, grp %d, error: 0x%x", nGrpId, s32Ret);
        }

        for (nChn = 0; nChn < pEntryParam->nIvpsChnCnt; nChn++) {
            s32Ret = AX_IVPS_DisableChn(nGrpId, nChn);
            if (AX_SUCCESS != s32Ret) {
                ALOGE("AX_IVPS_DisableChn failed, grp %d, chn %d, error: 0x%x", nGrpId, nChn, s32Ret);
            }
        }

        s32Ret = AX_IVPS_DestoryGrp(nGrpId);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("AX_IVPS_DestoryGrp failed, grp %d, error: 0x%x", nGrpId, s32Ret);
        }
    }

    ALOGI("SampleIvpsDestroy --");
    return s32Ret;
}

static AX_S32 SampleIvpsDeInit(AX_VOID)
{
    ALOGI("SampleIvpsDeInit ++");
    AX_S32 s32Ret = 0;

    s32Ret = AX_IVPS_Deinit();
    if (AX_SUCCESS != s32Ret) {
        ALOGE("AX_IVPS_Deinit failed, error: 0x%x", s32Ret);
    }
    ALOGI("SampleIvpsDeInit --");
    return s32Ret;
}

typedef struct AX_ALGO_BOX_T {
    // normalized coordinates
    AX_F32 fX;
    AX_F32 fY;
    AX_F32 fW;
    AX_F32 fH;
    AX_U32 nImgWidth;
    AX_U32 nImgHeight;
} AX_ALGO_BOX_T;

AX_S32 DrawRect(AX_SKEL_RESULT_T *pstResult, AX_S32 nIvpsChn){
    AX_U8 nRotation = 0;

    // rotation: 90
    if (g_entry_param.nVinIvpsMode == AX_GDC_ONLINE_VPP && g_entry_param.nGdcOnlineVppTest) {
        nRotation = 1;
    }

    return AX_OSD_DrawRect(pstResult, nIvpsChn, nRotation);
}

static AX_BOOL SkelResult2DetectResult(const AX_SKEL_RESULT_T *pstResult) {
    AX_BOOL bFound = AX_FALSE;
    for (AX_U32 i = 0; i < pstResult->nObjectSize; ++i) {
        AX_SKEL_OBJECT_ITEM_T *pstObjectItems = pstResult->pstObjectItems + i;
        if ((pstObjectItems->eTrackState == AX_SKEL_TRACK_STATUS_NEW) ||
            (pstObjectItems->eTrackState == AX_SKEL_TRACK_STATUS_UPDATE)) {
            if (strcmp(pstObjectItems->pstrObjectCategory, "body") == 0) {
                bFound = AX_TRUE;
                break;
            }
        }
    }

    return bFound;
}

static AX_VOID ChangeAiispMode(AX_S32 nCamId, AX_BOOL bAov) {
    if (g_entry_param.bUseTispInAov) {
        AX_ISP_IQ_SCENE_PARAM_T tIspSceneParam = {0};
        AX_ISP_IQ_GetSceneParam(gCams[nCamId].nPipeId, &tIspSceneParam);
        if (bAov) {
            // manual tisp
            tIspSceneParam.nAutoMode = 0;
            tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_DISABLE;
        } else {
            // default setting
            switch (g_entry_param.nAiIspMode) {
                case E_SNS_AIISP_MANUAL_SCENE_AIISP_MODE_E:
                    // manual mode
                    tIspSceneParam.nAutoMode = 0;
                    tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_ENABLE;
                    break;
                case E_SNS_AIISP_MANUAL_SCENE_TISP_MODE_E:
                    // manual mode
                    tIspSceneParam.nAutoMode = 0;
                    tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_DISABLE;
                    break;
                case E_SNS_AIISP_DEFAULT_SCENE_MODE_E:
                case E_SNS_AIISP_AUTO_SCENE_MODE_E:
                    // auto mode
                    tIspSceneParam.nAutoMode = 1;
                    break;
                default:
                    break;
            }
        }
        AX_ISP_IQ_SetSceneParam(gCams[nCamId].nPipeId, &tIspSceneParam);
    }
}

static AX_S32 UpdateVencRc(SAMPLE_ENTRY_PARAM_T *pEntryParam, AX_BOOL bAOV) {
    AX_S32 nVencChn = 0, nRet = 0;
    AX_S32 nBitrate = 0;
    if (!pEntryParam->bVencEnable) {
        return 0;
    }
    for (nVencChn = 0; nVencChn < pEntryParam->nIvpsChnCnt*pEntryParam->nCamCnt; nVencChn++) {
        if (!gOutChnAttr[nVencChn].bVenc) {
            continue;
        }

        AX_S32 nInWidth = gOutChnAttr[nVencChn].nWidth;
        AX_S32 nInHeight = gOutChnAttr[nVencChn].nHeight;

        if (pEntryParam->nGdcOnlineVppTest) {
            AX_S32 nTmp = nInWidth;
            nInWidth = nInHeight;
            nInHeight = nTmp;
        }

        AX_VENC_RC_PARAM_T stRcParam;
        memset(&stRcParam, 0, sizeof(stRcParam));
        nRet = AX_VENC_GetRcParam(nVencChn, &stRcParam);
        if (AX_SUCCESS != nRet) {
            ALOGE("Venc[%d]: AX_VENC_GetRcParam failed, error: 0x%x", nVencChn, nRet);
            continue;
        }

        if (bAOV) {
            if (pEntryParam->nVencRcChangePolicy == 0) {
                // AOV时码率修改成原始值的3倍
                switch (stRcParam.enRcMode)
                {
                case AX_VENC_RC_MODE_H264CBR:
                    nBitrate = GetVencBitrate(PT_H264, nInWidth, nInHeight);
                    stRcParam.stH264Cbr.u32BitRate = nBitrate * 3;
                    break;
                case AX_VENC_RC_MODE_H264VBR:
                    nBitrate = GetVencBitrate(PT_H264, nInWidth, nInHeight);
                    stRcParam.stH264Vbr.u32MaxBitRate = nBitrate * 3;
                    break;
                case AX_VENC_RC_MODE_H264FIXQP:
                    nBitrate = GetVencBitrate(PT_H264, nInWidth, nInHeight);
                    break;
                case AX_VENC_RC_MODE_H265CBR:
                    nBitrate = GetVencBitrate(PT_H265, nInWidth, nInHeight);
                    stRcParam.stH265Cbr.u32BitRate = nBitrate * 3;
                    break;
                case AX_VENC_RC_MODE_H265VBR:
                    nBitrate = GetVencBitrate(PT_H265, nInWidth, nInHeight);
                    stRcParam.stH265Vbr.u32MaxBitRate = nBitrate * 3;
                    break;
                case AX_VENC_RC_MODE_H265FIXQP:
                    nBitrate = GetVencBitrate(PT_H265, nInWidth, nInHeight);
                    break;
                default:
                    break;
                }
            } else {
                // AOV时GOP = 2, fps = 1
                stRcParam.stFrameRate.fSrcFrameRate = 1;
                stRcParam.stFrameRate.fDstFrameRate = 1;
                switch (stRcParam.enRcMode)
                {
                case AX_VENC_RC_MODE_H264CBR:
                    stRcParam.stH264Cbr.u32Gop = pEntryParam->nGopInAov;
                    break;
                case AX_VENC_RC_MODE_H264VBR:
                    stRcParam.stH264Vbr.u32Gop = pEntryParam->nGopInAov;
                    break;
                case AX_VENC_RC_MODE_H264FIXQP:
                    stRcParam.stH264FixQp.u32Gop = pEntryParam->nGopInAov;
                    break;
                case AX_VENC_RC_MODE_H265CBR:
                    stRcParam.stH265Cbr.u32Gop = pEntryParam->nGopInAov;
                    break;
                case AX_VENC_RC_MODE_H265VBR:
                    stRcParam.stH265Vbr.u32Gop = pEntryParam->nGopInAov;
                    break;
                case AX_VENC_RC_MODE_H265FIXQP:
                    stRcParam.stH265FixQp.u32Gop = pEntryParam->nGopInAov;
                    break;
                default:
                    break;
                }
            }

        } else {
            if (pEntryParam->nVencRcChangePolicy == 0) {
                switch (stRcParam.enRcMode)
                {
                case AX_VENC_RC_MODE_H264CBR:
                    nBitrate = GetVencBitrate(PT_H264, nInWidth, nInHeight);
                    stRcParam.stH264Cbr.u32BitRate = nBitrate;
                    break;
                case AX_VENC_RC_MODE_H264VBR:
                    nBitrate = GetVencBitrate(PT_H264, nInWidth, nInHeight);
                    stRcParam.stH264Vbr.u32MaxBitRate = nBitrate;
                    break;
                case AX_VENC_RC_MODE_H264FIXQP:
                    nBitrate = GetVencBitrate(PT_H264, nInWidth, nInHeight);
                    break;
                case AX_VENC_RC_MODE_H265CBR:
                    nBitrate = GetVencBitrate(PT_H265, nInWidth, nInHeight);
                    stRcParam.stH265Cbr.u32BitRate = nBitrate;
                    break;
                case AX_VENC_RC_MODE_H265VBR:
                    nBitrate = GetVencBitrate(PT_H265, nInWidth, nInHeight);
                    stRcParam.stH265Vbr.u32MaxBitRate = nBitrate;
                    break;
                case AX_VENC_RC_MODE_H265FIXQP:
                    nBitrate = GetVencBitrate(PT_H265, nInWidth, nInHeight);
                    break;
                default:
                    break;
                }
            } else {
                stRcParam.stFrameRate.fSrcFrameRate = pEntryParam->nSensorFrameRate;
                stRcParam.stFrameRate.fDstFrameRate = pEntryParam->nSensorFrameRate;
                switch (stRcParam.enRcMode)
                {
                case AX_VENC_RC_MODE_H264CBR:
                    stRcParam.stH264Cbr.u32Gop = pEntryParam->nSensorFrameRate * 2;
                    break;
                case AX_VENC_RC_MODE_H264VBR:
                    stRcParam.stH264Vbr.u32Gop = pEntryParam->nSensorFrameRate * 2;
                    break;
                case AX_VENC_RC_MODE_H264FIXQP:
                    stRcParam.stH264FixQp.u32Gop = pEntryParam->nSensorFrameRate * 2;
                    break;
                case AX_VENC_RC_MODE_H265CBR:
                    stRcParam.stH265Cbr.u32Gop = pEntryParam->nSensorFrameRate * 2;
                    break;
                case AX_VENC_RC_MODE_H265VBR:
                    stRcParam.stH265Vbr.u32Gop = pEntryParam->nSensorFrameRate * 2;
                    break;
                case AX_VENC_RC_MODE_H265FIXQP:
                    stRcParam.stH265FixQp.u32Gop = pEntryParam->nSensorFrameRate * 2;
                    break;
                default:
                    break;
                }
            }
        }

        nRet = AX_VENC_SetRcParam(nVencChn, &stRcParam);
        if (AX_SUCCESS != nRet) {
            ALOGE("Venc[%d]: AX_VENC_SetRcParam failed, error: 0x%x", nVencChn, nRet);
            continue;
        }
    }

    return 0;
}

static AX_S32 UpdateSnsFps(SAMPLE_ENTRY_PARAM_T *pEntryParam, AX_S32 nFps) {
    AX_S32 nRet = 0;
    AX_VIN_PIPE_ATTR_T tPipeAttr;
    AX_SNS_ATTR_T tSnsAttr = {0};
    pthread_mutex_lock(&g_mutex);

    for (AX_S32 i = 0; i < pEntryParam->nCamCnt; i++) {
        // just for test
        ChangeAiispMode(i, (nFps == 0) ? AX_TRUE : AX_FALSE);

#if defined(SAMPLE_SNS_SC200AI_TRIPLE)
        if (gCams[i].bMipiSwitchEnable) {
            AX_S32 nCurSns = gCams[i].nSwitchSnsId;
            if (nCurSns == 1) {
                AX_ISP_SetSnsActive(gCams[i].nPipeId, 2, (nFps == 0) ? AX_FALSE : AX_TRUE);
            } else {
                AX_ISP_SetSnsActive(gCams[i].nPipeId, 1, (nFps == 0) ? AX_FALSE : AX_TRUE);
            }
        }
#endif
        // change sns fps
        AX_F32 fFrameRate = SENSOR_FRAMERATE;
#if defined(SAMPLE_SNS_SC200AI_TRIPLE)
        if (gCams[i].bMipiSwitchEnable) {
            AX_S32 nCurSns = gCams[i].nSwitchSnsId;
            if (nCurSns == 1) {
                if (gCams[i].ptSnsHdl && gCams[i].ptSnsHdl->pfn_sensor_get_fps) {
                    nRet = gCams[i].ptSnsHdl->pfn_sensor_get_fps(1, &fFrameRate);
                    if (nRet != 0) {
                        ALOGE("[%d][1]pfn_sensor_get_fps failed, error: 0x%x", i, nRet);
                    }
                }
            } else {
                if (gCams[i].ptSnsHdl1 && gCams[i].ptSnsHdl1->pfn_sensor_get_fps) {
                    nRet = gCams[i].ptSnsHdl1->pfn_sensor_get_fps(2, &fFrameRate);
                    if (nRet != 0) {
                        ALOGE("[%d][2]pfn_sensor_get_fps failed, error: 0x%x", i, nRet);
                    }
                }
            }
        }
#else
        if (gCams[i].ptSnsHdl && gCams[i].ptSnsHdl->pfn_sensor_get_fps) {
            nRet = gCams[i].ptSnsHdl->pfn_sensor_get_fps(gCams[i].nPipeId, &fFrameRate);
            if (nRet != 0) {
                ALOGE("[%d]pfn_sensor_get_fps failed, error: 0x%x", i, nRet);
            }
        }
#endif

        nRet = AX_ISP_GetSnsAttr(gCams[i].nPipeId, &tSnsAttr);
        if (nRet != 0) {
            ALOGE("[%d]AX_ISP_GetSnsAttr failed, error: 0x%x", i, nRet);
        }

        if (IS_FPS_EQUAL(fFrameRate, tSnsAttr.fFrameRate)) {
            if (nFps == SENSOR_FRAMERATE) {
                ALOGI("change sns[%d] fps: %d->%d", gCams[i].nPipeId, (AX_S32)fFrameRate, SENSOR_FRAMERATE);
                tSnsAttr.fFrameRate = SENSOR_FRAMERATE;
            } else {
                ALOGI("change sns[%d] fps: %d->%d", gCams[i].nPipeId, (AX_S32)fFrameRate, SENSOR_MAX_FRAMERATE);
                tSnsAttr.fFrameRate = SENSOR_MAX_FRAMERATE;
            }

            nRet = AX_ISP_SetSnsAttr(gCams[i].nPipeId, &tSnsAttr);
            if (nRet != 0) {
                ALOGE("[%d]AX_ISP_SetSnsAttr failed, error: 0x%x", i, nRet);
            }
        } else {
            ALOGI("current sns[%d] fps is %d, need not change fps", gCams[i].nPipeId, (AX_S32)fFrameRate);
        }

        // change pipe fps ctrl
        memset(&tPipeAttr, 0x00, sizeof(tPipeAttr));
        nRet = AX_VIN_GetPipeAttr(gCams[i].nPipeId, &tPipeAttr);
        if (nRet != 0) {
            ALOGE("[%d]AX_VIN_GetPipeAttr failed, error: 0x%x", i, nRet);
            continue;
        }
        ALOGI("change pipe[%d] fps: %d->%d", gCams[i].nPipeId, (AX_S32)(tPipeAttr.tFrameRateCtrl.fDstFrameRate), nFps);
        tPipeAttr.tFrameRateCtrl.fDstFrameRate = nFps;
        tPipeAttr.tFrameRateCtrl.fSrcFrameRate = (nFps == SENSOR_FRAMERATE) ? SENSOR_FRAMERATE : SENSOR_MAX_FRAMERATE;
        nRet = AX_VIN_SetPipeAttr(gCams[i].nPipeId, &tPipeAttr);
        if (nRet != 0) {
            ALOGE("[%d]AX_VIN_SetPipeAttr failed, error: 0x%x", i, nRet);
        }
    }

    if (nFps == 0) {
        // change g_entry_param.nDetectFrameMax in sleep_thread_func, before sleep
        g_nLastFps = 1;
        UpdateVencRc(pEntryParam, AX_TRUE);
    } else {
        g_entry_param.nDetectFrameMax = DET_MAX_FRAME_NOBODY;
        g_nLastFps = SENSOR_FRAMERATE;
        UpdateVencRc(pEntryParam, AX_FALSE);
    }

    pthread_mutex_unlock(&g_mutex);

    return 0;
}

static AX_VOID QSCheckFlag() {
    if (g_nQSType == -1) {
        if (access(GetFlagFile(E_FLAG_FILE_REBOOT), F_OK) == 0) {
            g_nQSType = QSReboot;
        } else if (access(GetFlagFile(E_FLAG_FILE_AOV), F_OK) == 0) {

            // 启动15s后使能AOV，目的是让系统初始化全部完成，系统进入稳定的工作状态
            if (GetTickCountPts() - g_u64LaunchPts < 15000000) {
                return;
            }

            GetYuvForQRCode();

            // init capnum
            if (g_entry_param.nCapNum != 0) {
                for (AX_S32 i = 0; i < g_entry_param.nCamCnt; i++) {
                    AX_S32 nRet = AX_VIN_SetCapFrameNumbers(gCams[i].nPipeId, g_entry_param.nCapNum);
                    if (nRet == AX_SUCCESS) {
                        ALOGI("AX_VIN_SetCapFrameNumbers: pipe=%d, num=%d.", gCams[i].nPipeId, g_entry_param.nCapNum);
                    } else {
                        ALOGE("AX_VIN_SetCapFrameNumbers: pipe=%d, num=%d, failed, error=0x%x", gCams[i].nPipeId, g_entry_param.nCapNum, nRet);
                    }
                }
            }

            g_nRunningTimes = 1;
            g_nQSType = QSAOV;
            QS_VideoRecorderStart();
        }
    }
}

static AX_BOOL CheckStopFlag() {
    FILE *pFile = NULL;
    if (g_nQSType == QSReboot) {
        pFile = fopen(GetFlagFile(E_FLAG_FILE_REBOOT), "rt");
    } else if (g_nQSType == QSAOV){
        pFile = fopen(GetFlagFile(E_FLAG_FILE_AOV), "rt");
    } else {
        return AX_FALSE;
    }

    if (pFile != NULL) {
        AX_CHAR szBuf[64] = {0};
        fgets(szBuf, 63, pFile);
        fclose(pFile);
        szBuf[strcspn(szBuf, "\n")] = '\0';
        if (strcmp(szBuf, "stop") == 0) {
            return AX_TRUE;
        }
    }
    return AX_FALSE;
}

static AX_VOID NoBodyFoundNotify() {

    QSCheckFlag();

    static AX_BOOL bLogPrinted = AX_FALSE;

    if (g_nQSType == QSReboot) {
        // for debug
        if (CheckStopFlag()) {
            return;
        }

        if (!ThreadLoopStateGet() && !bLogPrinted) {
            if (g_entry_param.nRebootInterval == 0) {
                ALOGI("Detect no body found over %d frames, will reboot after H264 was saved.", g_entry_param.nDetectFrameMax);
            } else {
                ALOGI("Detect no body found over %d frames, will reboot until timeout(%ds).", g_entry_param.nDetectFrameMax, g_entry_param.nRebootInterval);
            }

            bLogPrinted = AX_TRUE;
        }
        g_NeedRebootSystem = AX_TRUE;

    } else if (g_nQSType == QSAOV) {
        // for debug
        if (CheckStopFlag()) {
            if (g_nLastFps == 1) {
                UpdateSnsFps(&g_entry_param, SENSOR_FRAMERATE);
            }

            AX_OSD_SetAovStatus(AX_FALSE);

#ifdef QSDEMO_AUDIO_SUPPORT
            if ((g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_CAPTURE)
                    && !g_entry_param.bStoreAudioInAov) {
                COMMON_AUDIO_SetCaptureAttr(AX_TRUE);
            }
#endif

            return;
        }

        AX_S32 nLastFps = g_nLastFps;
        AX_S32 nDetectNum = g_entry_param.nDetectFrameMax;

        if (g_nLastFps == SENSOR_FRAMERATE) {
            UpdateSnsFps(&g_entry_param, 0);
        }

        AX_OSD_SetAovStatus(AX_TRUE);

#ifdef QSDEMO_AUDIO_SUPPORT
        if ((g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_CAPTURE)
                && !g_entry_param.bStoreAudioInAov) {
            COMMON_AUDIO_SetCaptureAttr(AX_FALSE);
        }
#endif

        pthread_mutex_lock(&g_mutex);
        if (!g_bSleepFlag) {
            ALOGI("Detect no body found over %d frames, will goto sleep", nDetectNum);
            g_bSleepFlag = AX_TRUE;
            update_sleep_condition(CONDITION_MASK_SKEL, true);
            if (nLastFps != g_nLastFps) {
                // not wait venc
                update_sleep_condition(CONDITION_MASK_VENC0, true);
                if (g_entry_param.nCamCnt > 1) {
                    update_sleep_condition(CONDITION_MASK_VENC1, true);
                }
            }
        }
        pthread_mutex_unlock(&g_mutex);
    }
}

static AX_S32 SampleRgnInit(SAMPLE_ENTRY_PARAM_T *pEntryParam) {
    return AX_OSD_Init(pEntryParam);
}

static AX_S32 SampleRgnDestroy(SAMPLE_ENTRY_PARAM_T *pEntryParam) {
    ALOGI("SampleRgnDestroy ++");

    AX_OSD_DeInit(pEntryParam);

    ALOGI("SampleRgnDestroy --");
    return 0;
}

static AX_VOID SkelResultCallback(AX_SKEL_HANDLE pHandle, AX_SKEL_RESULT_T *pstResult, AX_VOID *private) {
    if (!pstResult) {
        return;
    }

    if (g_nQSType == QSAOV && g_nLastFps == 1) {
        // for debug: timestamp get skel result
        AX_SYS_SleepTimeStamp(AX_ID_USER, pstResult->nStreamId == 0 ? E_STS_GET_SKEL_0 : E_STS_GET_SKEL_1);
    }

    AX_BOOL bFoundBody = AX_FALSE;
    if (g_bNeedShowLauchResult &&
        g_stResultTimes.u64DetectTimeFromBoot[pstResult->nStreamId] == 0) {
        g_stResultTimes.u64DetectTimeFromBoot[pstResult->nStreamId] = GetTickCountPts();
        PrintLaunchTimeResult();
        ALOGW("Detect[%d]: 1st frame got detected result", pstResult->nStreamId);
    }
    // Do nothing when app has exited.
    if(ThreadLoopStateGet()) {
        return;
    }

    for (AX_S32 i = 0; i < IVPSChannelNumber; i++){
        if(i != ALGO_IVPS_CHN) {
            DrawRect(pstResult, i);
        }
    }

    bFoundBody = SkelResult2DetectResult(pstResult);
    if (bFoundBody) {
        gDetectFrames[pstResult->nStreamId] = 0;
        if (g_nLastFps == 1 && !g_bSleepFlag) {
            SET_IGNORE(pstResult->nStreamId);
            UpdateSnsFps(&g_entry_param, SENSOR_FRAMERATE);

            AX_OSD_SetAovStatus(AX_FALSE);

#ifdef QSDEMO_AUDIO_SUPPORT
            if ((g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_CAPTURE)
                    && !g_entry_param.bStoreAudioInAov) {
                // start audio capture
                COMMON_AUDIO_SetCaptureAttr(AX_TRUE);
            }

            // start audio play
            SampleStartAudioFilePlay(SAMPLE_AUDIO_FILE_TYPE_MONITOR);
#endif
        }
    } else {
        ++gDetectFrames[pstResult->nStreamId];
        if (g_nLastFps == 1) {
            ALOGW("Detect[%d]: nobody found, frameid=%llu", pstResult->nStreamId, pstResult->nFrameId);
        }

        if (g_entry_param.nCamCnt == 1) {
            if (gDetectFrames[pstResult->nStreamId] >= g_entry_param.nDetectFrameMax) {
                NoBodyFoundNotify();
                gDetectFrames[pstResult->nStreamId] = 0;
            }
        } else {
            if (gDetectFrames[0] >= g_entry_param.nDetectFrameMax && gDetectFrames[1] >= g_entry_param.nDetectFrameMax) {
                NoBodyFoundNotify();
                gDetectFrames[0] = 0;
                gDetectFrames[1] = 0;
            }
        }
    }
}

static AX_VOID UpdateMdResult(AX_S32 nCamId, AX_BOOL bFound) {
    if (g_bNeedShowLauchResult &&
        g_stResultTimes.u64DetectTimeFromBoot[nCamId] == 0) {
        g_stResultTimes.u64DetectTimeFromBoot[nCamId] = GetTickCountPts();
        PrintLaunchTimeResult();
        ALOGW("Detect[%d]: 1st frame got detected result", nCamId);
    }
    // Do nothing when app has exited.
    if(ThreadLoopStateGet()) {
        return;
    }

    if (bFound) {
        gDetectFrames[nCamId] = 0;
        if (g_nLastFps == 1) {
            SET_IGNORE(nCamId);
            UpdateSnsFps(&g_entry_param, SENSOR_FRAMERATE);

            AX_OSD_SetAovStatus(AX_FALSE);

#ifdef QSDEMO_AUDIO_SUPPORT
            if ((g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_CAPTURE)
                    && !g_entry_param.bStoreAudioInAov) {
                // start audio capture
                COMMON_AUDIO_SetCaptureAttr(AX_TRUE);
            }

            // start audio play
            SampleStartAudioFilePlay(SAMPLE_AUDIO_FILE_TYPE_MONITOR);
#endif
        }
    } else {
        ++gDetectFrames[nCamId];

        if (g_entry_param.nCamCnt == 1) {
            if (gDetectFrames[nCamId] >= g_entry_param.nDetectFrameMax) {
                NoBodyFoundNotify();
                gDetectFrames[nCamId] = 0;
            }
        } else {
            if (gDetectFrames[0] >= g_entry_param.nDetectFrameMax && gDetectFrames[1] >= g_entry_param.nDetectFrameMax) {
                NoBodyFoundNotify();
                gDetectFrames[0] = 0;
                gDetectFrames[1] = 0;
            }
        }
    }
}

#if defined(AX_RISCV_LOAD_MODEL_SUPPORT)
AX_VOID SampleSetReleaseModelImageMemFlag(AX_S32 nCam) {
    pthread_mutex_lock(&g_mtxReleaseImageMem);
    g_release_model_image_mem_flag |= (1 << nCam);
    pthread_cond_broadcast(&g_condReleaseImageMem);
    pthread_mutex_unlock(&g_mtxReleaseImageMem);
}

static AX_VOID *release_model_image_mem_thread_func(AX_VOID *pThreadParam) {
    pthread_mutex_lock(&g_mtxReleaseImageMem);
    while (g_release_model_image_mem_flag != 0xFF) {
        pthread_cond_wait(&g_condReleaseImageMem, &g_mtxReleaseImageMem);
    }
    pthread_mutex_unlock(&g_mtxReleaseImageMem);

    // release model image memory when init completely
    AX_S32 s32Ret = AX_SYS_ModelMemRelease();
    if (s32Ret) {
        ALOGW("AX_SYS_ModelMemRelease failed, error: 0x%x", s32Ret);
    } else {
        ALOGD("AX_SYS_ModelMemRelease done");
    }

    return NULL;
}

static AX_S32 SampleReleaseModelImageMemInit(AX_S32 nCamCount) {
    g_release_model_image_mem_flag = 0xFF;
    for (AX_S32 i = 0; i < nCamCount; i++) {
        g_release_model_image_mem_flag &= ~(1 << i);
    }

    pthread_t tid = 0;
    if (0 != pthread_create(&tid, NULL, release_model_image_mem_thread_func, NULL)) {
        return -1;
    }
    pthread_detach(tid);

    return 0;
}
#endif

static AX_S32 SampleSkelInit() {
    AX_S32 s32Ret = 0;
    AX_SKEL_INIT_PARAM_T stInitParam = {0};
    stInitParam.pStrModelDeploymentPath = "/opt/etc/skelModels";
    s32Ret = AX_SKEL_Init(&stInitParam);
    if(s32Ret != 0) {
        ALOGE("AX_SKEL_Init error, ret:0x%x", s32Ret);
        return s32Ret;
    }

    // create handle
    AX_SKEL_HANDLE_PARAM_T stHandleParam = {0};
    stHandleParam.ePPL = AX_SKEL_PPL_HVCP;
    stHandleParam.nFrameDepth = g_entry_param.nCamCnt;
    stHandleParam.nFrameCacheDepth = 1;
    stHandleParam.nIoDepth = 0;
    stHandleParam.nWidth = 1024;
    stHandleParam.nHeight = 576;

    // config settings (if need)
    AX_SKEL_CONFIG_T stConfig = {0};
    AX_SKEL_CONFIG_ITEM_T stItems[16] = {0};
    AX_U8 itemIndex = 0;
    stConfig.nSize = 0;
    stConfig.pstItems = &stItems[0];

    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stTrackEnableThreshold = {0};
    AX_SKEL_COMMON_THRESHOLD_CONFIG_T stPushEnableThreshold = {0};

    // detect only (disable track + disable push)
    {
        // track_disable
        stConfig.pstItems[itemIndex].pstrType = (AX_CHAR *)"track_enable";
        stTrackEnableThreshold.fValue = 0;
        stConfig.pstItems[itemIndex].pstrValue = (AX_VOID *)&stTrackEnableThreshold;
        stConfig.pstItems[itemIndex].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
        itemIndex++;

        // push_disable
        stConfig.pstItems[itemIndex].pstrType = (AX_CHAR *)"push_enable";
        stPushEnableThreshold.fValue = 0;
        stConfig.pstItems[itemIndex].pstrValue = (AX_VOID *)&stPushEnableThreshold;
        stConfig.pstItems[itemIndex].nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);
        itemIndex++;
    }

    if (itemIndex > 0) {
        stConfig.nSize = itemIndex;
        stHandleParam.stConfig = stConfig;
    }

    s32Ret = AX_SKEL_Create(&stHandleParam, &g_pSkelHandle);
    if(s32Ret != 0){
        ALOGE("AX_SKEL_Create error, ret:0x%x", s32Ret);
    } else {
        AX_SKEL_RegisterResultCallback(g_pSkelHandle, SkelResultCallback, NULL);
    }

    return s32Ret;
}

static AX_S32 SampleMdInit() {
    AX_S32 s32Ret = 0;
    AX_MD_CHN_ATTR_T stChnAttr;
    MD_CHN chn = 0;
    s32Ret = AX_IVES_Init();
    if(s32Ret != 0) {
        ALOGE("AX_IVES_Init error, ret:0x%x", s32Ret);
        return s32Ret;
    }

    s32Ret = AX_IVES_MD_Init();
    if(s32Ret != 0) {
        ALOGE("AX_IVES_MD_Init error, ret:0x%x", s32Ret);
        return s32Ret;
    }

    for (chn = 0 ; chn < g_entry_param.nCamCnt; chn++) {
        AX_S32 nWidth = gOutChnAttr[chn*IVPSChannelNumber + ALGO_IVPS_CHN].nWidth;
        AX_S32 nHeight = gOutChnAttr[chn*IVPSChannelNumber + ALGO_IVPS_CHN].nHeight;

        if (g_entry_param.nVinIvpsMode == AX_GDC_ONLINE_VPP && g_entry_param.nGdcOnlineVppTest) {
            AX_S32 nTmp = nWidth;
            nWidth = nHeight;
            nHeight = nTmp;
        }

        stChnAttr.mdChn = chn;
        stChnAttr.stArea.u32X = 0;
        stChnAttr.stArea.u32Y = 0;
        stChnAttr.stArea.u32W = nWidth;
        stChnAttr.stArea.u32H = nHeight;
        stChnAttr.stMbSize.u32W = 32;
        stChnAttr.stMbSize.u32H = 32;
        stChnAttr.enAlgMode = AX_MD_MODE_REF;
        stChnAttr.u8ThrY = 10;

        s32Ret = AX_IVES_MD_CreateChn(chn, & stChnAttr);
        if(s32Ret != 0) {
            ALOGE("AX_IVES_MD_CreateChn[%d] error, ret:0x%x", chn, s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}

static AX_S32 SampleMdDeInit() {
    AX_S32 s32Ret = 0;
    MD_CHN chn = 0;

    for (chn = 0 ; chn < g_entry_param.nCamCnt; chn++) {
        pthread_join(g_DetectThrdParam[chn].nTid, NULL);
    }

    for (chn = 0 ; chn < g_entry_param.nCamCnt; chn++) {
        s32Ret = AX_IVES_MD_DestoryChn(chn);
        if(s32Ret != 0) {
            ALOGE("AX_IVES_MD_CreateChn[%d] error, ret:0x%x", chn, s32Ret);
        }
    }

    s32Ret = AX_IVES_MD_DeInit();
    if(s32Ret != 0) {
        ALOGE("AX_IVES_MD_Init error, ret:0x%x", s32Ret);
    }

    s32Ret = AX_IVES_DeInit();
    if(s32Ret != 0) {
        ALOGE("AX_IVES_Init error, ret:0x%x", s32Ret);
    }

    return s32Ret;
}

static AX_S32 SampleSkelDestroy()
{
    ALOGI("SampleSkelDestroy ++");
    if (g_pSkelHandle) {
        for(AX_S32 i = 0; i < g_entry_param.nCamCnt; i++) {
            pthread_join(g_DetectThrdParam[i].nTid, NULL);
        }
        AX_SKEL_Destroy(g_pSkelHandle);
        g_pSkelHandle =  NULL;
    }
    ALOGI("SampleSkelDestroy --");
    return 0;
}

static AX_S32 SampleSkelDeInit()
{
    ALOGI("SampleSkelDeInit ++");
    AX_SKEL_DeInit();
    ALOGI("SampleSkelDeInit --");
    return 0;
}

#ifdef QSDEMO_AUDIO_SUPPORT
AX_VOID AudioStreamCallback(const AX_AUDIO_STREAM_T *ptStream, AX_VOID *pUserData) {
    if (ptStream) {
        AX_U8 *pStream = ptStream->pStream;
        AX_U32 u32Len = ptStream->u32Len;
        AX_U64 u64TimeStamp = ptStream->u64TimeStamp;

        for (AX_U32 i = 0; i < g_entry_param.nCamCnt; i++) {
            /* storage */
            QS_SaveAudio(i, pStream, u32Len, u64TimeStamp, AX_FALSE);
        }

        AX_U32 nChnNum = g_entry_param.nCamCnt * IVPSChannelNumber;
        for (AX_U32 i = 0; i < nChnNum; i++) {
            /* send to RTSP */
            if (gOutChnAttr[i].nRtspChn != -1) {
                AX_Rtsp_SendAudio(g_hRtsp, gOutChnAttr[i].nRtspChn, pStream, u32Len, u64TimeStamp);
            }
        }
    }
}

static AX_VOID *AudioFilePlayTriggerThread(AX_VOID* param) {
    AX_CHAR szName[50] = {0};
    sprintf(szName, "qs_aplay");
    prctl(PR_SET_NAME, szName);

    while (!gLoopExit) {
        // wait play
        pthread_mutex_lock(&g_mutex_trigger_audio_file_play);
        while (!gLoopExit
                && (g_sample_audio_file_type == SAMPLE_AUDIO_FILE_TYPE_NONE)) {
            pthread_cond_wait(&g_cond_trigger_audio_file_play, &g_mutex_trigger_audio_file_play);
        }
        pthread_mutex_unlock(&g_mutex_trigger_audio_file_play);

        if (!gLoopExit
            && g_sample_audio_file_type != SAMPLE_AUDIO_FILE_TYPE_NONE) {
            const AX_CHAR* pstrRes = NULL;
            AX_PAYLOAD_TYPE_E eType = PT_G711A;

            if (g_sample_audio_file_type == SAMPLE_AUDIO_FILE_TYPE_MONITOR) {
                pstrRes = "/customer/qsres/monitor_16k_mono.g711a";
                eType = PT_G711A;
            }

            AX_S32 s32Ret = -1;

            if (pstrRes) {
                s32Ret = COMMON_AUDIO_PlayFile(eType,
                                                pstrRes,
                                                1,
                                                AudioPlayCallback,
                                                NULL);
            }

            g_sample_audio_file_type = SAMPLE_AUDIO_FILE_TYPE_NONE;

            if (s32Ret == 0) {
                update_sleep_condition(CONDITION_MASK_AUDIO, false);
            }
        }
    }

    return (AX_VOID *)0;
}

static AX_S32 AudioFilePlayTriggerThreadStart() {
    if (g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_PLAY) {
        if (0 != pthread_create(&g_sample_audio_file_play_tid, NULL, AudioFilePlayTriggerThread, NULL)) {
            return -1;
        }
    }

    return 0;
}

static AX_S32 SampleStartAudioFilePlay(AX_U32 nAudioType) {
    if (g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_PLAY) {
        pthread_mutex_lock(&g_mutex_trigger_audio_file_play);
        g_sample_audio_file_type = nAudioType;
        pthread_cond_broadcast(&g_cond_trigger_audio_file_play);
        pthread_mutex_unlock(&g_mutex_trigger_audio_file_play);
    }

    return 0;
}

static AX_S32 SampleAudioInit()
{
    AX_S32 s32Ret = 0;

    ALOGI("audio-flag=%d", g_entry_param.nAudioFlag);

    if (g_entry_param.nAudioFlag != SAMPLE_AUDIO_FLAG_NONE) {
        ALOGI("SampleAudioInit ++");

        const SAMPLE_AUDIO_CAPTURE_ENTRY_PARAM_T* pACapEntry = NULL;
        const SAMPLE_AUDIO_PLAY_ENTRY_PARAM_T* pAPlayEntry = NULL;

        if (g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_CAPTURE) {
            pACapEntry = qs_audio_capture_config();
        }

        if (g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_PLAY) {
            pAPlayEntry = qs_audio_play_config();
        }

        s32Ret = COMMON_AUDIO_Init(pACapEntry, pAPlayEntry, AudioStreamCallback, NULL);

        if (s32Ret == 0) {
            if (pAPlayEntry) {
                AudioFilePlayTriggerThreadStart();
            }
        }

        ALOGI("SampleAudioInit --");
    }

    return s32Ret;
}

static AX_S32 SampleAudioDeInit()
{
    AX_S32 s32Ret = 0;

    if (g_entry_param.nAudioFlag != SAMPLE_AUDIO_FLAG_NONE) {
        ALOGI("SampleAudioDeInit ++");

        SampleStartAudioFilePlay(SAMPLE_AUDIO_FILE_TYPE_NONE);

        if (g_sample_audio_file_play_tid) {
            pthread_join(g_sample_audio_file_play_tid, NULL);
            g_sample_audio_file_play_tid = 0;
        }

        s32Ret = COMMON_AUDIO_Deinit();

        ALOGI("SampleAudioDeInit --");
    }

    return s32Ret;
}
#endif

static AX_S32 SampleLinkInit(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    size_t nMax = ARRAY_SIZE(gLinkModAttr);
    for (size_t i = 0; i < nMax; i++) {
        AX_SYS_Link(&gLinkModAttr[i].srcMod, &gLinkModAttr[i].dstMod);
   }
    return 0;
}

static AX_S32 SampleLinkDeInit(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    ALOGI("SampleLinkDeInit ++");
    size_t nMax = ARRAY_SIZE(gLinkModAttr);
    for (size_t i = 0; i < nMax; i++) {
        AX_SYS_UnLink(&gLinkModAttr[i].srcMod, &gLinkModAttr[i].dstMod);
    }
    ALOGI("SampleLinkDeInit --");
    return 0;
}

static AX_BOOL SampleCheckIP() {
    AX_CHAR pszIP[64] = {0};
    if (GetLocalIP(pszIP, 64) && (strcmp(pszIP, "127.0.0.1") != 0)) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

static AX_S32 SampleRtspInit(AX_S32 nChnNum)
{
    if (!g_entry_param.bRtspEnable) {
        return 0;
    }

    AX_S32 nChn = 0;
    AX_RTSP_ATTR_T stAttr[MAX_RTSP_MAX_CHANNEL_NUM];

    memset(&stAttr[0], 0x00, sizeof(stAttr));
    for (nChn = 0; nChn < nChnNum; nChn++) {
        stAttr[nChn].nChannel = nChn;
        stAttr[nChn].stVideoAttr.bEnable = AX_TRUE;
        stAttr[nChn].stVideoAttr.ePt = PT_H264;

#ifdef QSDEMO_AUDIO_SUPPORT
        if (g_entry_param.nAudioFlag & SAMPLE_AUDIO_FLAG_CAPTURE) {
            SAMPLE_AUDIO_ENCODER_INFO_T stEncoderInfo;
            AX_S32 s32Ret = COMMON_AUDIO_GetEncoderInfo(&stEncoderInfo);

            if (!s32Ret
                && stEncoderInfo.bEnable) {
                stAttr[nChn].stAudioAttr.bEnable = AX_TRUE;
                stAttr[nChn].stAudioAttr.ePt = stEncoderInfo.ePt;
                stAttr[nChn].stAudioAttr.nSampleRate = stEncoderInfo.nSampleRate;
                stAttr[nChn].stAudioAttr.nChnCnt = stEncoderInfo.nChnCnt;
                stAttr[nChn].stAudioAttr.nAOT = stEncoderInfo.nAOT;
            }
        }
#endif
    }

    AX_Rtsp_Init(&g_hRtsp, stAttr, nChnNum, 0);
    AX_Rtsp_Start(g_hRtsp);

    return 0;
}

static AX_S32 SampleRtspDestroy(AX_VOID)
{
    ALOGI("SampleRtspDestroy ++");
    if (g_hRtsp) {
        AX_Rtsp_Stop(g_hRtsp);
    }

    ALOGI("SampleRtspDestroy --");
    return 0;
}

static AX_S32 SampleRtspDeInit(AX_VOID)
{
    ALOGI("SampleRtspDeInit ++");
    if (g_hRtsp) {
        AX_Rtsp_Deinit(g_hRtsp);
        g_hRtsp = NULL;
    }

    ALOGI("SampleRtspDeInit --");
    return 0;
}

static void *InitVinPostProc(void *arg)
{
    ALOGD("Vin Post Proc started");

    prctl(PR_SET_NAME, "qs_initvinpost");

    SAMPLE_ENTRY_PARAM_T *pEntryParam = (SAMPLE_ENTRY_PARAM_T *)arg;
    AX_U64 u64LastPts = GetTickCountPts();
    AX_S32 s32Ret = SampleLinkInit(pEntryParam);
    if (s32Ret) {
        ALOGE("SampleLinkInit failed, error: 0x%x", s32Ret);
         goto ivps_signal;
    }
    ALOGD("link init done");

    s32Ret = AX_IVPS_Init();
    if (AX_SUCCESS != s32Ret) {
        ALOGE("AX_IVPS_Init failed, error: 0x%x", s32Ret);
        goto ivps_signal;
    }
    s32Ret = SampleIvpsInit(pEntryParam);
    if (AX_SUCCESS != s32Ret) {
        ALOGE("AX_IVPS_SetPipelineAttr failed, error: 0x%x", s32Ret);
        goto ivps_signal;
    }
    ALOGD("ivps init done");

    s32Ret = SampleVencInit(pEntryParam);
    if (AX_SUCCESS != s32Ret) {
        ALOGE("SampleVencInit failed, error: 0x%x", s32Ret);
        goto ivps_signal;
    }
    ALOGD("venc init done");

    if (pEntryParam->bJencEnable) {
        s32Ret = SampleJencInit(pEntryParam);
        if (AX_SUCCESS != s32Ret) {
            ALOGE("SampleJencInit failed, error: 0x%x", s32Ret);
            goto ivps_signal;
        }
        ALOGD("jenc init done");
    } else {
        ALOGD("jenc is disabled");
    }

    pthread_mutex_lock(&g_mtxIvpsInit);
    g_bIvpsInited = AX_TRUE;
    pthread_cond_broadcast(&g_condIvpsInit);
    pthread_mutex_unlock(&g_mtxIvpsInit);

    SampleRgnInit(pEntryParam);
    ALOGD("rgn init done");

    if (pEntryParam->bDetectEnable) {
        if (pEntryParam->nDetetAlgo == 0) {
            ALOGI("detect-algo=%d(%s)", g_entry_param.nDetetAlgo, "skel");

            s32Ret = SampleSkelInit();
            if(s32Ret != 0) {
                ALOGE("SampleSkelInit failed");
                goto vinpost_fail;
            }
            ALOGD("detect init done");

        } else if (pEntryParam->nDetetAlgo == 1) {
            ALOGI("detect-algo=%d(%s)", g_entry_param.nDetetAlgo, "md");

            s32Ret = SampleMdInit();
            if(s32Ret != 0) {
                ALOGE("SampleMdInit failed");
                goto vinpost_fail;
            }
            ALOGD("detect init done");

        } else if (pEntryParam->nDetetAlgo == 2) {
            ALOGI("detect-algo=%d(%s)", g_entry_param.nDetetAlgo, "md->skel");

            s32Ret = SampleSkelInit();
            if(s32Ret != 0) {
                ALOGE("SampleSkelInit failed");
                goto vinpost_fail;
            }
            ALOGD("detect init done");

            s32Ret = SampleMdInit();
            if(s32Ret != 0) {
                ALOGE("SampleMdInit failed");
                goto vinpost_fail;
            }
        } else {
            ALOGI("detect-algo=%d(%s)", g_entry_param.nDetetAlgo, "invalid");
            goto vinpost_fail;
        }

        DetectThreadStart(pEntryParam);
    }

    UpdateRunTime(&g_stResultTimes.u64DetectInit, &u64LastPts);

    g_bVinPostInited = AX_TRUE;
    return NULL;

ivps_signal:
    pthread_mutex_lock(&g_mtxIvpsInit);
    g_bIvpsInited = AX_TRUE;
    pthread_cond_broadcast(&g_condIvpsInit);
    pthread_mutex_unlock(&g_mtxIvpsInit);

vinpost_fail:
    g_bVinPostInited = AX_TRUE;
    return NULL;
}

static void SampleVinPostInit(SAMPLE_ENTRY_PARAM_T *pEntryParam) {
    pthread_t tid = 0;
    pthread_create(&tid, NULL, InitVinPostProc, (AX_VOID *)pEntryParam);
    pthread_detach(tid);
}

static AX_VOID *GetFrameThreadForAI(AX_VOID *pThreadParam)
{
    DETECT_GET_THREAD_PARAM_T *t = (DETECT_GET_THREAD_PARAM_T *)pThreadParam;
    IVPS_GRP IvpsGrp = t->nIvpsGrp;
    IVPS_CHN IvpsChn = t->nIvpsChn;

    AX_VIDEO_FRAME_T tVideoFrame;
    AX_U64 frameid = 0;
    AX_U64 u64CurPts = 0;
    AX_U64 u64SeqNo = 0;
    AX_MD_MB_THR_T stThrs;
    AX_MD_MB_SAD_T stSad;
    AX_IVES_CCBLOB_T stBlob;
    AX_S32 nMdConfidence = 30;

    AX_CHAR szName[50] = {0};
    sprintf(szName, "qs_ivps_ai_%d_%d", IvpsGrp, IvpsChn);
    prctl(PR_SET_NAME, szName);

    while (!gLoopExit) {
        memset(&tVideoFrame, 0, sizeof(AX_VIDEO_FRAME_T));
        AX_S32 ret = AX_IVPS_GetChnFrame(IvpsGrp, IvpsChn, &tVideoFrame, 200);
        if (0 != ret) {
            if (u64CurPts != 0) {
                if ((GetTickCountPts() - u64CurPts) > 2000000) {
                    ALOGI("SKEL[%d] had not got frame over 2s, last seqno=%llu", IvpsGrp, u64SeqNo);
                    u64CurPts = 0;
                }
            }
            if (AX_ERR_IVPS_FLOW_END == ret) {
                break;
            } else if (AX_ERR_IVPS_BUF_EMPTY == ret || AX_ERR_IVPS_TIMED_OUT == ret || AX_ERR_IVPS_UNKNOWN == ret) {
                MSSleep(10);
                continue;
            } else {
                ALOGE("SKEL[%d] AX_IVPS_GetChnFrame(grp%d,chn%d) failed, err=0x%x", IvpsGrp, IvpsGrp, IvpsChn, ret);
            }

            MSSleep(10);
            continue;
        }
        if (g_nQSType == QSAOV && g_nLastFps == 1) {
            // for debug: timestamp get skel result
            AX_SYS_SleepTimeStamp(AX_ID_USER, IvpsGrp == 0 ? E_STS_GET_FRAME_0 : E_STS_GET_FRAME_1);
        }

        u64CurPts = GetTickCountPts();
        u64SeqNo = tVideoFrame.u64SeqNum;

        pthread_mutex_lock(&g_mutex);
        if (g_nQSType == QSAOV && g_nLastFps == 1) {
            ALOGI("SKEL[%d] get yuv to detect, seqno=%llu, frameid=%llu", IvpsGrp, u64SeqNo, frameid);
            g_bSleepFlag = AX_FALSE;
        }
        pthread_mutex_unlock(&g_mutex);

        if (g_entry_param.nDetetAlgo == 0) { // skel
            if (g_pSkelHandle) {
                if (frameid == 0) {
                    ALOGD("Detect[%d] 1st frame seqno: %llu.", IvpsGrp, tVideoFrame.u64SeqNum);
                }
                AX_SKEL_FRAME_T skel_frame = {0};
                skel_frame.nStreamId = IvpsGrp;
                skel_frame.nFrameId = frameid;
                skel_frame.stFrame = tVideoFrame;
                if (frameid== 0 && g_stResultTimes.u64DetectYUVTimeFromBoot[IvpsGrp] == 0) {
                    g_stResultTimes.u64DetectYUVTimeFromBoot[IvpsGrp] = GetTickCountPts();
                }

                ret = AX_SKEL_SendFrame(g_pSkelHandle, &skel_frame, 200);
                ++frameid;
            }
        } else if (g_entry_param.nDetetAlgo == 1) { // md
            if (frameid == 0) {
                ALOGD("Detect[%d] 1st frame seqno: %llu.", IvpsGrp, tVideoFrame.u64SeqNum);
            }
            stThrs.u32Count = 0;
            stThrs.pMbThrs = NULL;
            stSad.u32Count = 0;
            stSad.pMbSad = NULL;

            ret = AX_IVES_MD_Process(IvpsGrp, (AX_IVES_IMAGE_T *)&tVideoFrame, &stThrs, &stSad, &stBlob);

            if (g_nQSType == QSAOV && g_nLastFps == 1) {
                // for debug: timestamp get result from md
                AX_SYS_SleepTimeStamp(AX_ID_USER, IvpsGrp == 0 ? E_STS_GET_SKEL_0 : E_STS_GET_SKEL_1);
            }

            if (ret == 0) {
                AX_U32 nSumThrs = 0;
                for (AX_U32 k = 0; k < stThrs.u32Count; ++k) {
                    nSumThrs += stThrs.pMbThrs[k];
                }
                AX_BOOL bMDDetected = (nSumThrs >= nMdConfidence) ? AX_TRUE : AX_FALSE;
                UpdateMdResult(IvpsGrp, bMDDetected);
            }
            ++frameid;
        } else if (g_entry_param.nDetetAlgo == 2) { // md->skel
            // IvpsGrp = nStreamId
            if (g_nLastFps == SENSOR_FRAMERATE)
            { // skel after md->skel while md return true and skel found body.
                if (g_pSkelHandle) {
                    if (frameid == 0) {
                        ALOGD("Detect[%d] 1st frame seqno: %llu.", IvpsGrp, tVideoFrame.u64SeqNum);
                    }
                    AX_SKEL_FRAME_T skel_frame = {0};
                    skel_frame.nStreamId = IvpsGrp;
                    skel_frame.nFrameId = frameid;
                    skel_frame.stFrame = tVideoFrame;
                    if (frameid== 0 && g_stResultTimes.u64DetectYUVTimeFromBoot[IvpsGrp] == 0) {
                        g_stResultTimes.u64DetectYUVTimeFromBoot[IvpsGrp] = GetTickCountPts();
                    }
                    ret = AX_SKEL_SendFrame(g_pSkelHandle, &skel_frame, 200);
                    ++frameid;
                }
            }
            else
            {  // md->skel
                if (frameid == 0) {
                    ALOGD("Detect[%d] 1st frame seqno: %llu.", IvpsGrp, tVideoFrame.u64SeqNum);
                }
                stThrs.u32Count = 0;
                stThrs.pMbThrs = NULL;
                stSad.u32Count = 0;
                stSad.pMbSad = NULL;
                ret = AX_IVES_MD_Process(IvpsGrp, (AX_IVES_IMAGE_T *)&tVideoFrame, &stThrs, &stSad, &stBlob);
                if (ret == 0) {
                    AX_U32 nSumThrs = 0;
                    for (AX_U32 k = 0; k < stThrs.u32Count; ++k) {
                        nSumThrs += stThrs.pMbThrs[k];
                    }
                    AX_BOOL bMDDetected = (nSumThrs >= nMdConfidence) ? AX_TRUE : AX_FALSE;
                    // do skel proc
                    if (g_pSkelHandle && bMDDetected) {
                        AX_SKEL_FRAME_T skel_frame = {0};
                        skel_frame.nStreamId = IvpsGrp;
                        skel_frame.nFrameId = frameid;
                        skel_frame.stFrame = tVideoFrame;
                        if (frameid== 0 && g_stResultTimes.u64DetectYUVTimeFromBoot[IvpsGrp] == 0) {
                            g_stResultTimes.u64DetectYUVTimeFromBoot[IvpsGrp] = GetTickCountPts();
                        }
                        ret = AX_SKEL_SendFrame(g_pSkelHandle, &skel_frame, 200);
                        ++frameid;
                    } else {
                        UpdateMdResult(IvpsGrp, bMDDetected);
                        ++frameid;
                    }
                }
            }
        }

        ret = AX_IVPS_ReleaseChnFrame(IvpsGrp, IvpsChn, &tVideoFrame);
    }

    return NULL;
}


AX_S32 DetectThreadStart(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    for(AX_S32 i = 0; i < pEntryParam->nCamCnt; i++) {
        if (0 != pthread_create(&g_DetectThrdParam[i].nTid, NULL, GetFrameThreadForAI, (AX_VOID *)&g_DetectThrdParam[i])) {
            continue;
        }
    }
    return 0;
}

static AX_VOID GdcOnlineVppTest(AX_CAMERA_T *pCam) {

    // mirror and flip
    AX_S32 nRet = AX_VIN_SetPipeMirror(pCam->nPipeId, AX_TRUE);
    if (AX_SUCCESS != nRet) {
        ALOGE("AX_VIN_SetPipeMirror[%d] failed ret=0x%x", pCam->nPipeId, nRet);
    }
    nRet = AX_VIN_SetChnFlip(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, AX_TRUE);
    if (AX_SUCCESS != nRet) {
        ALOGE("AX_VIN_SetChnFlip[%d] failed ret=0x%x", pCam->nPipeId, nRet);
    }

    // rotation 270
    AX_VIN_ROTATION_E eRotation = AX_VIN_ROTATION_270;
    nRet = AX_VIN_SetChnRotation(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, eRotation);
    if (AX_SUCCESS != nRet) {
        ALOGE("AX_VIN_SetChnRotation[%d] failed ret=0x%x", pCam->nPipeId, nRet);
    }

    // ldc
    AX_ISP_IQ_LDC_PARAM_T tLdcAttr;
    AX_ISP_IQ_GetLdcParam(pCam->nPipeId, &tLdcAttr);
    tLdcAttr.nLdcEnable = AX_TRUE;
    tLdcAttr.nType = AX_ISP_IQ_LDC_TYPE_V1;
    tLdcAttr.tLdcV1Param.bAspect = AX_TRUE;
    tLdcAttr.tLdcV1Param.nXRatio = 0;
    tLdcAttr.tLdcV1Param.nYRatio = 0;
    tLdcAttr.tLdcV1Param.nXYRatio = 50;
    tLdcAttr.tLdcV1Param.nCenterXOffset = 0;
    tLdcAttr.tLdcV1Param.nCenterYOffset = 0;
    tLdcAttr.tLdcV1Param.nDistortionRatio = 2000;
    tLdcAttr.tLdcV1Param.nSpreadCoef = 0;

    nRet = AX_ISP_IQ_SetLdcParam(pCam->nPipeId, &tLdcAttr);
    if (AX_SUCCESS != nRet) {
        ALOGE("AX_ISP_IQ_SetLdcParam[%d] failed ret=0x%x", pCam->nPipeId, nRet);
    }

    // crop
    AX_F32 fEZoomRatio = 8;
    AX_VIN_CROP_INFO_T tCropInfo;
    AX_VIN_GetChnCrop(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, &tCropInfo);

    nRet = AX_VIN_GetChnRotation(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, &eRotation);
    AX_U32 nWidth = pCam->tChnAttr[AX_VIN_CHN_ID_MAIN].nWidth;
    AX_U32 nHeight = pCam->tChnAttr[AX_VIN_CHN_ID_MAIN].nHeight;
    if (AX_VIN_ROTATION_90 == eRotation || AX_VIN_ROTATION_270 == eRotation) {
        AX_U32 nTmp = nWidth;
        nWidth = nHeight;
        nHeight = nTmp;
    }

    AX_U32 nMinW = ceil(nWidth * 2.0 / AX_EZOOM_MAX);
    AX_U32 nMinH = ceil(nWidth * 2.0 / AX_EZOOM_MAX);

    AX_F32 nStepW = (nWidth - nMinW) * 1.0f / AX_EZOOM_MAX;
    AX_F32 nStepH = (nHeight - nMinH) * 1.0f / AX_EZOOM_MAX;

    tCropInfo.bEnable = AX_TRUE;
    tCropInfo.eCoordMode = AX_VIN_COORD_ABS;
    tCropInfo.tCropRect.nWidth = nMinW + ceil(nStepW * (AX_EZOOM_MAX - fEZoomRatio + 1));
    tCropInfo.tCropRect.nHeight = nMinH + ceil(nStepH * (AX_EZOOM_MAX - fEZoomRatio + 1));
    if (tCropInfo.tCropRect.nWidth > nWidth) {
        tCropInfo.tCropRect.nWidth = nWidth;
    }

    if (tCropInfo.tCropRect.nHeight > nHeight) {
        tCropInfo.tCropRect.nHeight = nHeight;
    }

    tCropInfo.tCropRect.nStartX = (nWidth - tCropInfo.tCropRect.nWidth) / 2;
    tCropInfo.tCropRect.nStartY = (nHeight - tCropInfo.tCropRect.nHeight) / 2;

    ALOGI("sns[%d] crop info enable=%d, coord_mode=%d, rect=[%d, %d, %d, %d]", pCam->nPipeId, tCropInfo.bEnable, tCropInfo.eCoordMode,
        tCropInfo.tCropRect.nStartX, tCropInfo.tCropRect.nStartY, tCropInfo.tCropRect.nWidth, tCropInfo.tCropRect.nHeight);

    nRet = AX_VIN_SetChnCrop(pCam->nPipeId, AX_VIN_CHN_ID_MAIN, &tCropInfo);
    if (AX_SUCCESS != nRet) {
        ALOGE("AX_VIN_SetChnCrop[%d] failed ret=0x%x", pCam->nPipeId, nRet);
    }
}

static AX_VOID *cam_open_thread_func(AX_VOID *pThreadParam) {

    AX_CAMERA_T *pCam = (AX_CAMERA_T *)pThreadParam;
    AX_ISP_IQ_SCENE_PARAM_T tIspSceneParam = {0};
    ALOGD("cam[%d] thread start", pCam->nNumber);

    prctl(PR_SET_NAME, "qs_cam_open");

    AX_S32 s32Ret = COMMON_CAM_Open(pCam, 1);
    if (s32Ret) {
        ALOGE("COMMON_CAM_Open [%d] fail, error: 0x%x", pCam->nNumber, s32Ret);
    }

    if (g_entry_param.nCamCnt == 1) {
        UpdateRunTime(&g_stResultTimes.u64CamOpen[pCam->nNumber], &g_stResultTimes.u64CamOpenLastPts);
    } else {
        AX_U64 u64LasPts = g_stResultTimes.u64CamOpenLastPts;
        UpdateRunTime(&g_stResultTimes.u64CamOpen[pCam->nNumber], &u64LasPts);
    }

    if (s32Ret == AX_SUCCESS) {
        s32Ret = AX_ISP_IQ_GetSceneParam(pCam->nPipeId, &tIspSceneParam);
        if (s32Ret == AX_SUCCESS) {
            switch (g_entry_param.nAiIspMode) {
                case E_SNS_AIISP_MANUAL_SCENE_AIISP_MODE_E:
                    // manual mode
                    tIspSceneParam.nAutoMode = 0;
                    tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_ENABLE;
                    break;
                case E_SNS_AIISP_MANUAL_SCENE_TISP_MODE_E:
                    // manual mode
                    tIspSceneParam.nAutoMode = 0;
                    tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_DISABLE;
                    break;
                case E_SNS_AIISP_AUTO_SCENE_MODE_E:
                    // auto mode
                    tIspSceneParam.nAutoMode = 1;
                    break;
                default:
                    break;
            }

            s32Ret = AX_ISP_IQ_SetSceneParam(pCam->nPipeId, &tIspSceneParam);
            if (s32Ret != AX_SUCCESS) {
                ALOGE("AX_ISP_IQ_SetSceneParam [%d] fail, error: 0x%x", pCam->nPipeId, s32Ret);
            }
        } else {
            ALOGE("AX_ISP_IQ_GetSceneParam [%d] fail, error: 0x%x", pCam->nPipeId, s32Ret);
        }
    }

    if (g_entry_param.nSlowShutter != -1 || g_entry_param.nAeManualShutter != -1) {
        AX_ISP_IQ_AE_PARAM_T tIspAeParam;
        memset(&tIspAeParam, 0x00, sizeof(tIspAeParam));

        s32Ret = AX_ISP_IQ_GetAeParam(pCam->nPipeId, &tIspAeParam);
        if (0 != s32Ret) {
            ALOGE("AX_ISP_IQ_GetAeParam failed, ret=0x%x.", s32Ret);
        }

        if (g_entry_param.nSlowShutter != -1) {
            tIspAeParam.tAeAlgAuto.tSlowShutterParam.nFrameRateMode = (g_entry_param.nSlowShutter == 0) ? 0 : 1;
            ALOGI("[%d]set slow shutter: %d.", pCam->nPipeId, tIspAeParam.tAeAlgAuto.tSlowShutterParam.nFrameRateMode);
        }

        if (g_entry_param.nAeManualShutter != -1) {
            tIspAeParam.nEnable = 0;
            tIspAeParam.tExpManual.nShutter = (AX_U32)g_entry_param.nAeManualShutter;
            ALOGI("[%d]set ae manual shutter: %d.", pCam->nPipeId, tIspAeParam.tExpManual.nShutter);
        }

        s32Ret = AX_ISP_IQ_SetAeParam(pCam->nPipeId, &tIspAeParam);
        if (0 != s32Ret) {
            ALOGE("AX_ISP_IQ_SetAeParam failed, ret=0x%x.", s32Ret);
        }
    }

    //just for test
    if (g_entry_param.nVinIvpsMode == AX_GDC_ONLINE_VPP && g_entry_param.nGdcOnlineVppTest) {
        GdcOnlineVppTest(pCam);
    }

    return NULL;
}

AX_S32 cam_open_parallel(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    for (AX_S32 i = 0; i < pEntryParam->nCamCnt; i++) {
        pthread_t tid = 0;
        if (0 != pthread_create(&tid, NULL, cam_open_thread_func, (AX_VOID *)&gCams[i])) {
            return -1;
        }
        pthread_detach(tid);
    }

    return 0;
}

AX_S32 cam_hdrmode_switch(AX_SNS_HDR_MODE_E eHdrMode)
{
    ALOGI("sns mode change %d to %d", g_entry_param.eHdrMode, eHdrMode);
    AX_ISP_IQ_SCENE_PARAM_T tIspSceneParam = {0};
    COMMON_CAM_Close(&gCams[0], g_entry_param.nCamCnt);

    g_entry_param.eHdrMode = eHdrMode;
    qs_cam_change_hdrmode(&g_entry_param, &gCams[0]);
    COMMON_CAM_Open(&gCams[0], g_entry_param.nCamCnt);

    for (AX_S32 i = 0; i < g_entry_param.nCamCnt; i++) {
        AX_ISP_IQ_GetSceneParam(gCams[i].nPipeId, &tIspSceneParam);
        switch (g_entry_param.nAiIspMode) {
            case E_SNS_AIISP_MANUAL_SCENE_AIISP_MODE_E:
                // manual mode
                tIspSceneParam.nAutoMode = 0;
                tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_ENABLE;
                break;
            case E_SNS_AIISP_MANUAL_SCENE_TISP_MODE_E:
                // manual mode
                tIspSceneParam.nAutoMode = 0;
                tIspSceneParam.tManualParam.nAiWorkMode = AX_AI_DISABLE;
                break;
            case E_SNS_AIISP_AUTO_SCENE_MODE_E:
                // auto mode
                tIspSceneParam.nAutoMode = 1;
                break;
            default:
                break;
        }

        AX_ISP_IQ_SetSceneParam(gCams[i].nPipeId, &tIspSceneParam);
    }

    return 0;
}

#if defined(SAMPLE_SNS_SC200AI_TRIPLE)
static AX_VOID *cam_zoom_func(AX_VOID *pParam) {
    prctl(PR_SET_NAME, "qs_zoom");
    AX_U32 nLoopCnt = MAX_SWITCH_ZOOM_RATIO * 10;
    for (AX_S32 i = 10; i <= nLoopCnt  && !ThreadLoopStateGet(); ) {
        if (i == 10) {
            for (AX_S32 j = 0; j < 200; j++) {
                MSSleep(50);
                if (ThreadLoopStateGet()) {
                    return NULL;
                }
            }
        }

        if (g_nLastFps == 1) {
            // continue in aov
            MSSleep(100);
            continue;
        }

        AX_F32 fEZoomRatio = (i * 1.0) / 10.0;
        if (fEZoomRatio > (g_entry_param.tEzoomSwitchInfo.fSwitchRatio - 0.05)
            && fEZoomRatio < (g_entry_param.tEzoomSwitchInfo.fSwitchRatio + 0.05)) {
            for (AX_S32 j = 0; j < 200; j++) {
                MSSleep(50);
                if (ThreadLoopStateGet()) {
                    return NULL;
                }
            }
        }

        COMMON_CAM_EZoomSwitch(&gCams[1], fEZoomRatio);

        if (fEZoomRatio < g_entry_param.tEzoomSwitchInfo.fSwitchRatio) {
            MSSleep(200);
        } else {
            MSSleep(100);
        }

        if (i == MAX_SWITCH_ZOOM_RATIO * 10) {
            i = 10;
        } else {
            i++;
        }
    }

    return NULL;
}

static AX_S32 StartZoomTestThread() {
    pthread_t tid = 0;
    pthread_create(&tid, NULL, cam_zoom_func, NULL);
    pthread_detach(tid);
    return 0;
}
#endif

// 检测VENC主码流在nDuration时间内是否正常出流，nDuration单位秒
static AX_BOOL CheckVencStream(AX_U64 nDuration) {
    nDuration = nDuration * 1000000; // us
    AX_U64 nCurPts = GetTickCountPts();
    AX_S32 nC0V0 = 0;     // cam 0 venc 0
    AX_S32 nC1V0 = g_entry_param.nIvpsChnCnt;  // cam 1 venc 0
    if (g_entry_param.nCamCnt == 1) {
        // 如果nDuration秒没有收到帧，则复制log
        if ((gGetStreamParam[nC0V0].u64GetVencFrmCnt == 0 && nCurPts > g_u64LaunchPts && (nCurPts - g_u64LaunchPts) >= nDuration) ||
            (gGetStreamParam[nC0V0].u64GetVencFrmCnt > 0 && nCurPts > gGetStreamParam[nC0V0].u64LastFramePts && (nCurPts - gGetStreamParam[nC0V0].u64LastFramePts) >= nDuration)) {
            return AX_FALSE;
        }
    } else {
        // 如果4秒没有收到帧，则复制log

        if (((gGetStreamParam[nC0V0].u64GetVencFrmCnt == 0 || gGetStreamParam[nC1V0].u64GetVencFrmCnt == 0) && (nCurPts - g_u64LaunchPts) >= nDuration) ||
            (gGetStreamParam[nC0V0].u64GetVencFrmCnt > 0 && nCurPts > gGetStreamParam[nC0V0].u64LastFramePts && (nCurPts - gGetStreamParam[nC0V0].u64LastFramePts) >= nDuration) ||
            (gGetStreamParam[nC1V0].u64GetVencFrmCnt > 0 && nCurPts > gGetStreamParam[nC1V0].u64LastFramePts && (nCurPts - gGetStreamParam[nC1V0].u64LastFramePts) >= nDuration)) {
            return AX_FALSE;
        }
    }

    return AX_TRUE;
}

static AX_S32 QSDemoRun(SAMPLE_ENTRY_PARAM_T *pEntryParam)
{
    AX_S32 s32Ret = 0;
    COMMON_SYS_ARGS_T tCommonArgs = {0};
    COMMON_SYS_ARGS_T tPrivArgs = {0};
    AX_ISP_AUX_INFO_T tAuxInfo = {0};
    AX_BOOL bPrinted = AX_FALSE;

    AX_U64 u64LastPts = g_u64LaunchPts;
    AX_BOOL bResultCopyed = AX_FALSE;
    AX_BOOL bRtspInited = AX_FALSE;
    AX_BOOL bCameraInited = AX_FALSE;
    AX_U64 u64LastRunTimes = 0;
#if defined(SAMPLE_SNS_SC200AI_TRIPLE)
    AX_U32 nCurSwitchSnsId = 1;
#endif

    /* Step1: cam config & pool Config */
    qs_cam_pool_config(pEntryParam, &tCommonArgs, &tPrivArgs, &gCams[0]);

    /* Step2: SYS Init */
    s32Ret = COMMON_SYS_Init(&tCommonArgs);
    if (s32Ret) {
        ALOGE("COMMON_SYS_Init fail, error: 0x%x", s32Ret);
        goto EXIT_FAIL;
    }
    ALOGD("sys and pool init done");
    UpdateRunTime(&g_stResultTimes.u64SysInit, &u64LastPts);

    AX_SYS_RegisterEventCb(AX_ID_USER, QSNotifyEventCallback, AX_NULL);
    StartSleepThread();

    for (AX_S32 i = 0; i < tCommonArgs.nCamCnt; i++) {
        s32Ret = AX_SYS_SetVINIVPSMode(gCams[i].nPipeId, i, (AX_VIN_IVPS_MODE_E)(pEntryParam->nVinIvpsMode));
        if (s32Ret) {
            ALOGE("AX_SYS_SetVINIVPSMode [%d -> %d] failed, error: 0x%x", gCams[i].nPipeId, i, s32Ret);
            goto EXIT_FAIL;
        }
        ALOGD("set itp online vpp done");
    }

    s32Ret = COMMON_NPU_Init();
    if (s32Ret) {
        ALOGE("COMMON_NPU_Init fail, error: 0x%x", s32Ret);
    }
    ALOGD("npu init done");
    UpdateRunTime(&g_stResultTimes.u64NpuInit, &u64LastPts);

#if defined(AX_RISCV_LOAD_MODEL_SUPPORT)
    if (pEntryParam->nHdrModeTest == 0) {
        /* start thread to wait for release model image memory */
        SampleReleaseModelImageMemInit(tCommonArgs.nCamCnt);
    }
#endif

    /* start thread to init ivps skel venc module */
    SampleVinPostInit(pEntryParam);

    ALOGD("vin and mipi init start");
    s32Ret = COMMON_CAM_Init();
    if (s32Ret) {
        ALOGE("COMMON_CAM_Init fail, error: 0x%x", s32Ret);
        goto EXIT_FAIL3;
    }
    bCameraInited = AX_TRUE;

    ALOGD("vin and mipi init done");
    UpdateRunTime(&g_stResultTimes.u64VinInit, &u64LastPts);

    s32Ret = COMMON_CAM_PrivPoolInit(&tPrivArgs);
    if (s32Ret) {
        ALOGE("COMMON_CAM_PrivPoolInit fail, error: 0x%x", s32Ret);
        goto EXIT_FAIL3;
    }
    ALOGD("vin priv pool init done");

    s32Ret = AX_ISP_GetAuxiliaryInfo(gCams[0].nPipeId, &tAuxInfo);
    if (s32Ret) {
        ALOGW("AX_ISP_GetAuxiliaryInfo failed, error: 0x%x", s32Ret);
    } else {
        ALOGW("current lux=%u", tAuxInfo.nLux);
    }

    QS_MonitorSDCardStart();
    ALOGD("sd card monitor started");

    QS_VideoRecorderInit(pEntryParam->nCamCnt, 4*1024*1024, 32 * 1024, pEntryParam->nMaxRecordFileCount);
    ALOGD("video recorder init done");

    g_stResultTimes.u64CamOpenLastPts = u64LastPts;
    cam_open_parallel(pEntryParam);

#ifdef QSDEMO_AUDIO_SUPPORT
    // audio init
    s32Ret = SampleAudioInit();

    if (s32Ret) {
        ALOGE("SampleAudioInit fail, error: 0x%x", s32Ret);
        goto EXIT_FAIL3;
    }
#endif

    AX_U32 nRound = 0;

    AX_U64 nCurPts = GetTickCountPts();
    AX_U64 nSwitchCount = pEntryParam->nHdrModeTest;

    AX_BOOL bSyslogChanged = AX_FALSE;

    while (!ThreadLoopStateGet()) {
        if(pEntryParam->nCamCnt == 1) {
            if (!bPrinted && g_stResultTimes.u64CamOpen[0] != 0 && g_stResultTimes.u64DetectInit != 0) {
                ALOGI("Init latency: total:%llu, sys:%llu, npu:%llu, isp:%llu, cam open:%llu, detect:%llu",
                        (g_stResultTimes.u64CamOpenLastPts - g_u64LaunchPts),
                        g_stResultTimes.u64SysInit,
                        g_stResultTimes.u64NpuInit,
                        g_stResultTimes.u64VinInit,
                        g_stResultTimes.u64CamOpen[0],
                        g_stResultTimes.u64DetectInit);
                bPrinted = AX_TRUE;
                QSCheckFlag();
            }
        } else {
            if (!bPrinted && g_stResultTimes.u64CamOpen[0] != 0 && g_stResultTimes.u64CamOpen[1] != 0 && g_stResultTimes.u64DetectInit != 0) {
                ALOGI("Init latency: total:%llu, sys:%llu, npu:%llu, isp:%llu, cam open:[%llu, %llu] detect:%llu",
                        (g_stResultTimes.u64CamOpenLastPts + (g_stResultTimes.u64CamOpen[0]>g_stResultTimes.u64CamOpen[1]?g_stResultTimes.u64CamOpen[0]:g_stResultTimes.u64CamOpen[1]) - g_u64LaunchPts),
                        g_stResultTimes.u64SysInit,
                        g_stResultTimes.u64NpuInit,
                        g_stResultTimes.u64VinInit,
                        g_stResultTimes.u64CamOpen[0],
                        g_stResultTimes.u64CamOpen[1],
                        g_stResultTimes.u64DetectInit);
                bPrinted = AX_TRUE;
                QSCheckFlag();
#if defined(SAMPLE_SNS_SC200AI_TRIPLE)
                ALOGI("AutoZoom enable: %d", pEntryParam->bAutoZoomLoopOn);
                if (pEntryParam->bAutoZoomLoopOn) {
                    StartZoomTestThread();
                }
#endif
            }
        }

        MSSleep(200);
        if (g_nQSType == QSReboot && g_nRunningTimes > 0) {
            if (QS_IsSDCardReady() && !bSyslogChanged && gGetStreamParam[0].u64GetVencFrmCnt > 60) {
                QS_ChangeSysLogPath(g_nRunningTimes);
                bSyslogChanged = AX_TRUE;
            }
        }

        if (!bResultCopyed) {
            if (!CheckVencStream(4)) {
                ALOGI("venc no frame output long time, copy result files");
                QS_CopyResultFiles((g_nQSType == QSAOV) ? AX_FALSE : AX_TRUE);
                bResultCopyed = AX_TRUE;
                u64LastRunTimes = g_nRunningTimes;
            }
        }

        if (!bRtspInited  && SampleCheckIP()) {
            SampleRtspInit(2 * pEntryParam->nCamCnt);
            bRtspInited = AX_TRUE;
            ALOGD("rtsp init done");
        }

        if (bResultCopyed && g_nQSType == QSAOV ) {
            if (g_nRunningTimes > u64LastRunTimes && u64LastRunTimes != 0) {
                bResultCopyed = AX_FALSE;
            }
        }

        if (++nRound > 10 * 30) {
            // Release memory back to the system every 30 seconds.
            // https://linux.die.net/man/3/malloc_trim
            malloc_trim(0);
            nRound = 0;
        }

        if (g_nQSType == -1 && (GetTickCountPts() - nCurPts) > 15000000) {
            nCurPts = GetTickCountPts();
            if (nSwitchCount > 0) {
                if (CheckVencStream(2)) {
                    cam_hdrmode_switch(pEntryParam->eHdrMode == AX_SNS_LINEAR_MODE ? AX_SNS_HDR_2X_MODE : AX_SNS_LINEAR_MODE);
                    --nSwitchCount;
                } else {
                    ALOGI("venc no frame output long time, stop hdr/sdr switch...");
                    nSwitchCount = 0;
                }
            }
        }
    }

    gLoopExit = 1;
    update_sleep_condition(CONDITION_MASK_STOP, true);

    while(!g_bVinPostInited) {
        MSSleep(100);
    }

    ALOGI("QSDemoRun is stopping...");
    if (g_nQSType == QSReboot && !bResultCopyed) {
        if (!QS_IsSDCardReady()) {
            MSSleep(3*1000);
            if (!QS_IsSDCardReady()) {
                ALOGI("QSDemoRun not MountSDCard");
            }
        }
        QS_StartCopyResultFiles();
    }

    AX_SYS_UnregisterEventCb(AX_ID_USER);

#ifdef QSDEMO_AUDIO_SUPPORT
    // audio deinit
    SampleAudioDeInit();
#endif

    // destory
    SampleRtspDestroy();
    SampleVencDestroy(pEntryParam);
    if (bCameraInited) {
        COMMON_CAM_Close(&gCams[0], tCommonArgs.nCamCnt);
        bCameraInited = AX_FALSE;
    }

    if (pEntryParam->bDetectEnable) {
        if (pEntryParam->nDetetAlgo == 0) { // SKEL
            SampleSkelDestroy();
        } else if (pEntryParam->nDetetAlgo == 1) { // MD
            SampleMdDeInit();
        } else if (pEntryParam->nDetetAlgo == 2) { // MD->SKEL
            SampleSkelDestroy();
            SampleMdDeInit();
        } else {
            ALOGI("QSDemoRun invalid DetetAlgo type");
        }

        SampleRgnDestroy(pEntryParam);
    }
    SampleIvpsDestroy(pEntryParam);

    // deinit
    SampleLinkDeInit(pEntryParam);
    if (pEntryParam->bDetectEnable) {
        SampleSkelDeInit();
    }
    SampleVencDeInit();
    SampleIvpsDeInit();

#if defined(SAMPLE_SNS_SC200AI_TRIPLE)
    srand(time(NULL));
    nCurSwitchSnsId = rand() % 2 + 1;
    QS_SetSnsSwitch(nCurSwitchSnsId);
    ALOGI("set switch sns id: %d", nCurSwitchSnsId);
#endif

EXIT_FAIL3:
    if (bCameraInited) {
        COMMON_CAM_Close(&gCams[0], tCommonArgs.nCamCnt);
        bCameraInited = AX_FALSE;
    }

    ALOGI("COMMON_CAM_Deinit ++");
    COMMON_CAM_Deinit();
    ALOGI("COMMON_CAM_Deinit --");
    COMMON_SYS_DeInit();
    SampleRtspDeInit();
    QS_VideoRecorderStop();
    QS_VideoRecorderDeinit();
    QS_StopCopyResultFiles();
    QS_MonitorSDCardStop();
EXIT_FAIL:
    ALOGI("QSDemoRun is stopped");
    return s32Ret;
}


static AX_VOID SigInt(AX_S32 signo)
{
    ALOGI("SigInt Catch signal %d", signo);
    gLoopExit = 1;
    ThreadLoopStateSet(AX_TRUE);
}

static AX_VOID SigStop(AX_S32 signo)
{
    ALOGI("SigStop Catch signal %d", signo);
    ThreadLoopStateSet(AX_TRUE);
}

static AX_VOID GetYuvForQRCode() {
    // sensor 与 ivps 初始化完毕，并正常出流

    // 从ivps grp0 chn0上取yuv， 分辨率是1280x720，是压缩的，需要crop resize成640x360非压缩
    IVPS_GRP IvpsGrp = 0;
    IVPS_CHN IvpsChn = 1;
    AX_U64 phyBuff = 0;
    AX_VOID* virBuff = NULL;
    AX_U32 frameSize = 640 * 360 * 3 / 2;
    AX_S32 nRet = 0;
    AX_U8 nOutFifoDepth = 0;
    AX_VIDEO_FRAME_T tSrcFrame;
    AX_VIDEO_FRAME_T tDstFrame;
    AX_IVPS_CROP_RESIZE_ATTR_T tCropResizeAttr;
    memset(&tSrcFrame, 0, sizeof(AX_VIDEO_FRAME_T));
    memset(&tDstFrame, 0, sizeof(AX_VIDEO_FRAME_T));

    nRet = AX_SYS_MemAlloc(&phyBuff, &virBuff, frameSize, 0, (AX_S8*)"QR_CODE_BUFF");
    if (nRet != AX_SUCCESS) {
        ALOGI("AX_SYS_MemAlloc failed, err=0x%x", nRet);
        return;
    }

    // 设置nOutFifoDepth为1
    AX_IVPS_PIPELINE_ATTR_T tPipelineAttr = {0};
    AX_IVPS_GetPipelineAttr(IvpsGrp, &tPipelineAttr);
    nOutFifoDepth = tPipelineAttr.nOutFifoDepth[IvpsChn];
    if (nOutFifoDepth == 0) {
        tPipelineAttr.nOutFifoDepth[IvpsChn] = 1;
        AX_IVPS_SetPipelineAttr(IvpsGrp, &tPipelineAttr);
    }

    // 获取yuv帧，这里超时时间设置了200ms
    nRet = AX_IVPS_GetChnFrame(IvpsGrp, IvpsChn, &tSrcFrame, 200);

    // 恢复nOutFifoDepth
    if (nOutFifoDepth == 0) {
        tPipelineAttr.nOutFifoDepth[IvpsChn] = nOutFifoDepth;
        AX_IVPS_SetPipelineAttr(IvpsGrp, &tPipelineAttr);
    }

    if (nRet == AX_SUCCESS) {
        memset(&tDstFrame, 0x00, sizeof(tDstFrame));
        tDstFrame.u32Width = 640;
        tDstFrame.u32Height = 360;
        tDstFrame.enImgFormat = AX_FORMAT_YUV420_SEMIPLANAR;
        tDstFrame.stCompressInfo.enCompressMode = AX_COMPRESS_MODE_NONE;
        tDstFrame.u32PicStride[0] = tDstFrame.u32Width;
        tDstFrame.u32PicStride[1] = tDstFrame.u32Width;
        tDstFrame.u32PicStride[2] = tDstFrame.u32Width;
        tDstFrame.u64PhyAddr[0] = phyBuff;
        tDstFrame.u64VirAddr[0] = (AX_ULONG)virBuff;
        tDstFrame.u32FrameSize = frameSize;

        memset(&tCropResizeAttr, 0, sizeof(tCropResizeAttr));
        tCropResizeAttr.tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_STRETCH;

        nRet = AX_IVPS_CropResizeVpp(&tSrcFrame, &tDstFrame, &tCropResizeAttr);

        AX_IVPS_ReleaseChnFrame(IvpsGrp, IvpsChn, &tSrcFrame);

        if (nRet == AX_SUCCESS) {
            // todo:  scan qr code
            // virBuff: is yuv(NV12) buffer, 640x360,

            // 下面是存文件验证OK
            FILE *pFile = fopen("/customer/640x360.yuv", "wb");
            if (pFile) {
                fwrite(virBuff, 1, frameSize, pFile);
                fclose(pFile);
            }

            ALOGI("scan qr code done");
            goto qr_exit;
        } else  {
            ALOGI("AX_IVPS_CropResizeVpp, err=0x%x", nRet);
            goto qr_exit;
        }

    } else {
        ALOGI("AX_IVPS_GetChnFrame, err=0x%x", nRet);
        goto qr_exit;
    }

qr_exit:
    AX_SYS_MemFree(phyBuff, virBuff);
    return;
}

AX_VOID PrintHelp()
{
    printf("CMD:\n");

    printf("\n\t-a(optional): Enable AIISP\n");
    printf("\t\t0: tisp\n");
    printf("\t\t1: aiisp(system default scene mode)\n");
    printf("\t\t2: aiisp(manual scene mode to aiisp)\n");
    printf("\t\t3: aiisp(manual scene mode to tisp)\n");
    printf("\t\t4: aiisp(auto scene mode)\n");

    printf("\n\t-b(optional): vin ivps mode\n");
    printf("\t\t1: gdc online vpp\n");
    printf("\t\t2: itp onle vpp(default)\n");
    printf("\t\t2: aiisp(manual scene mode to aiisp)\n");
    printf("\t\t3: aiisp(manual scene mode to tisp)\n");
    printf("\t\t4: aiisp(auto scene mode)\n");

    printf("\n\t-c(optional): Reboot Interval from main lanch\n");
    printf("\t\t\tunit sencond\n");
    printf("\t\t\tneed insert sd card\n");
    printf("\t\t0: reboot if no body found 5fps(default)\n");
    printf("\t\t>0: reboot if timeout n sencods\n");

    printf("\n\t-d(optional): sdr/hdr switch times\n");
    printf("\t\t0: no switch(default)\n");
    printf("\t\t>0: switch N times, interval 60s\n");

    printf("\n\t-e(optional): enable jenc\n");
    printf("\t\t0: disable and not init jenc\n");
    printf("\t\t1: enable(default)\n");

    printf("\n\t-f(optional): set aov vin cap num when wakeup\n");
    printf("\t\t0: 0: not set, defaut 1fps(default)\n");
    printf("\t\t1~3: cap num\n");
    printf("\t\t4: for test,  randam switch between 1~3\n");

    printf("\n\t-g(optional): sc850sl output 2M flag\n");
    printf("\t\t0: disable(default)\n");
    printf("\t\t1: eanble\n");
    printf("\t\tif enable, need set env and reboot: ax_env.sh set qs_sc850sl2m 1\n");

    printf("\n\t-i(optional): audio flag\n");
    printf("\t\t0: disable(default)\n");
    printf("\t\t1: eanble\n");

    printf("\n\t-j(optional): rtsp enable\n");
    printf("\t\t0: disable\n");
    printf("\t\t1: eanble(default)\n");

    printf("\n\t-k(optional): aov sleep time\n");
    printf("\t\t\tunit ms, default 800ms\n");

    printf("\n\t-l(optional): vin channel frame mode\n");
    printf("\t\t0: off\n");
    printf("\t\t1: ring (default)\n");

    printf("\n\t-m(optional): detect algo\n");
    printf("\t\t0: skel (default)\n");
    printf("\t\t1: md\n");
    printf("\t\t2: md->skel\n");

    printf("\n\t-n(optional): gdc online vpp test\n");
    printf("\t\t0: disable(default)\n");
    printf("\t\t1: eanble\n");

    printf("\n\t-o(optional): use tisp in aov\n");
    printf("\t\t0: disable(default)\n");
    printf("\t\t1: eanble\n");

    printf("\n\t-p(optional): store auido in aov\n");
    printf("\t\t0: disable(default)\n");
    printf("\t\t1: eanble\n");

    printf("\n\t-q(optional): change venc rc policy in aov\n");
    printf("\t\t0: change bitrate(default)\n");
    printf("\t\t1: change gop\n");

    printf("\n\t-r(optional): max record file count\n");
    printf("\t\t5: default)\n");

    printf("\n\t-s(optional): venc gop in aov\n");
    printf("\t\t10: default)\n");
}

int main(int argc, char *argv[])
{
    g_u64LaunchPts = GetTickCountPts();
    ALOGW("Build at %s %s", __DATE__, __TIME__);

    int c;
    int isExit = 0;
    AX_S32 nValue = 0;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SigInt);
    signal(SIGTSTP, SigStop);

    while ((c = getopt(argc, argv, "a:b:c:d:e:f:g:i:j:k:l:m:n:o:p:q:r:s:t:u:v:w:h")) != -1) {
        isExit = 0;
        switch (c) {
        case 'a':
            nValue = (AX_S32)atoi(optarg);
            if (nValue >= E_SNS_TISP_MODE_E && nValue <= E_SNS_AIISP_AUTO_SCENE_MODE_E) {
                g_entry_param.nAiIspMode = nValue;
            } else {
                ALOGW("AiIspMode=%d is invalid, use default value(%d)", nValue, g_entry_param.nAiIspMode);
            }
            break;
        case 'b':
            nValue = (AX_S32)atoi(optarg);
            if (nValue == 1 || nValue == 2) {
                g_entry_param.nVinIvpsMode = nValue;
            } else {
                ALOGW("VinIvpsMode=%d is invalid, use default value(%d)", nValue, g_entry_param.nVinIvpsMode);
            }
            break;
        case 'c':
            nValue = (AX_S32)atoi(optarg);
            if (nValue >= 0) {
                g_entry_param.nRebootInterval = nValue;
            } else {
                ALOGW("RebootInterval=%d is invalid, use default value(%d)", nValue, g_entry_param.nRebootInterval);
            }
            break;
        case 'd':
            nValue = (AX_S32)atoi(optarg);
            if (nValue >= 0) {
                g_entry_param.nHdrModeTest = nValue;
            } else {
                ALOGW("HdrModeTest=%d is invalid, use default value(%d)", nValue, g_entry_param.nHdrModeTest);
            }
            break;
        case 'e':
            nValue = (AX_S32)atoi(optarg);
            g_entry_param.bJencEnable = (nValue == 0) ? AX_FALSE : AX_TRUE;
            break;
        case 'f':
            nValue = (AX_S32)atoi(optarg);
            if (nValue >= 0 && nValue <= 4) {
                g_entry_param.nCapNum = nValue;
                ALOGI("capnum=%d.", g_entry_param.nCapNum);
                if (nValue == 4) {
                    g_entry_param.nCapNum = 1;
                    g_entry_param.bRandomCapNum = AX_TRUE;
                }
            } else {
                ALOGW("CapNum=%d is invalid, use default value(%d)", nValue, g_entry_param.nCapNum);
            }
            break;
        case 'g':
            nValue = (AX_S32)atoi(optarg);
            g_entry_param.bSc850sl2M = (nValue == 0) ? AX_FALSE : AX_TRUE;
            break;
        case 'i':
            nValue = (AX_S32)atoi(optarg);
            if (nValue >= 0 && nValue <= 3) {
                g_entry_param.nAudioFlag = nValue;
            } else {
                ALOGW("AudioFlag=%d is invalid, use default value(%d)", nValue, g_entry_param.nAudioFlag);
            }
            break;
        case 'j':
            nValue = (AX_S32)atoi(optarg);
            g_entry_param.bRtspEnable = (nValue == 0) ? AX_FALSE : AX_TRUE;
            break;
        case 'k':
            nValue = (AX_S32)atoi(optarg);
            if (nValue >= 0) {
                g_entry_param.nSleepTime = nValue;
            } else {
                ALOGW("SleepTime=%d is invalid, use default value(%d)", nValue, g_entry_param.nSleepTime);
            }
            break;
        case 'l':
            nValue = (AX_S32)atoi(optarg);
            if (nValue == 0 || nValue == 1) {
                g_entry_param.nVinChnFrmMode = nValue;
            } else {
                ALOGW("VinChnFrmMode=%d is invalid, use default value(%d)", nValue, g_entry_param.nVinChnFrmMode);
            }
            break;
        case 'm':
            nValue = (AX_S32)atoi(optarg);
            if (nValue >= 0 && nValue <= 2) {
                g_entry_param.nDetetAlgo = nValue;
            } else {
                ALOGW("DetetAlgo=%d is invalid, use default value(%d)", nValue, g_entry_param.nDetetAlgo);
            }
            break;
        case 'n':
            nValue = (AX_S32)atoi(optarg);
            g_entry_param.nGdcOnlineVppTest = (nValue == 0) ? AX_FALSE : AX_TRUE;
            break;
        case 'o':
            nValue = (AX_S32)atoi(optarg);
            g_entry_param.bUseTispInAov = (nValue == 0) ? AX_FALSE : AX_TRUE;
            break;
        case 'p':
            nValue = (AX_S32)atoi(optarg);
            g_entry_param.bStoreAudioInAov = (nValue == 0) ? AX_FALSE : AX_TRUE;
            break;
        case 'q':
            nValue = (AX_S32)atoi(optarg);
            if (nValue >= 0 && nValue <= 1) {
                g_entry_param.nVencRcChangePolicy = nValue;
            } else {
                ALOGW("nVencRcChangePolicy=%d is invalid, use default value(%d)", nValue, g_entry_param.nVencRcChangePolicy);
            }
            break;
        case 'r':
            nValue = (AX_S32)atoi(optarg);
            g_entry_param.nMaxRecordFileCount = (nValue > 0) ? nValue : g_entry_param.nMaxRecordFileCount;
            break;
        case 's':
            nValue = (AX_S32)atoi(optarg);
            g_entry_param.nGopInAov = (nValue > 0) ? nValue : g_entry_param.nGopInAov;
            break;
        case 't':
            {
                if (sscanf(optarg, "%d,%d,%d,%d,%hd,%hd", &g_entry_param.tEzoomSwitchInfo.nX,
                                                        &g_entry_param.tEzoomSwitchInfo.nY,
                                                        &g_entry_param.tEzoomSwitchInfo.nWidth,
                                                        &g_entry_param.tEzoomSwitchInfo.nHeight,
                                                        &g_entry_param.tEzoomSwitchInfo.nCx,
                                                        &g_entry_param.tEzoomSwitchInfo.nCy) != 6) {
                    ALOGW("Invalid ezoom_switch_info: %s", optarg);
                }
            }
            break;
        case 'u':
            g_entry_param.nSlowShutter = (AX_S32)atoi(optarg);
            break;
        case 'v':
            g_entry_param.bAutoZoomLoopOn = ((AX_S32)atoi(optarg) == 0) ? AX_FALSE : AX_TRUE;
            break;
        case 'w':
            g_entry_param.nAeManualShutter = (AX_S32)atoi(optarg);
            break;
        case 'h':
        default:
            isExit = 1;
            break;
        }
    }

    if (isExit) {
        PrintHelp();
        exit(0);
    }

    QS_SetMalloptPolicy();

#if defined(SAMPLE_SNS_SC200AI_DOUBLE) || defined(SAMPLE_SNS_SC850SL_OS04A10)
    if (g_entry_param.nVencDumpNum > 30) {
        g_entry_param.nVencDumpNum = 30;
    }
#endif
    LogOpen();
    pthread_mutex_init(&g_mutex, NULL);
    ALOGI("aiisp-mode=%d", g_entry_param.nAiIspMode);

#if defined(SAMPLE_SNS_SC850SL_SINGLE)
    if (g_entry_param.bSc850sl2M) {
        g_entry_param.bSc850sl2M = QS_GetSc850sl2MFlag(g_entry_param.bSc850sl2M);
        ALOGI("sc850sl2m=%d", g_entry_param.bSc850sl2M);
    } else {
        ALOGI("sc850sl2m=0");
    }
#endif

#if defined(SAMPLE_SNS_SC200AI_TRIPLE)
    g_entry_param.nVinIvpsMode = AX_GDC_ONLINE_VPP;
    g_entry_param.nSwitchSnsId = QS_GetSnsSwitch(g_entry_param.nSwitchSnsId);
    ALOGI("get switch sns id: %d", g_entry_param.nSwitchSnsId);
#endif

    ALOGI("hdrmode-test=%d", g_entry_param.nHdrModeTest);
    if (g_entry_param.nHdrModeTest > 0) {
        g_entry_param.eHdrMode = AX_SNS_LINEAR_MODE;
    }

    ALOGI("vinivps-mode=%d", g_entry_param.nVinIvpsMode);

    if (g_entry_param.nVinIvpsMode == AX_GDC_ONLINE_VPP) {
        ALOGI("chnfrm-mode=%d", g_entry_param.nVinChnFrmMode);
        ALOGI("gdconlinevpp-test=%d", g_entry_param.nGdcOnlineVppTest);
    } else {
        g_entry_param.nGdcOnlineVppTest = 0;
        g_entry_param.nVinChnFrmMode = 0;
    }

    ALOGI("reboot-interval=%d", g_entry_param.nRebootInterval);
    ALOGI("tisp-in-aov=%d", g_entry_param.bUseTispInAov);

    QSDemoRun(&g_entry_param);

    pthread_mutex_destroy(&g_mutex);
    LogClose();

    if (g_NeedRebootSystem) {
        ALOGI("reboot...");
        QS_RunCmd("reboot", NULL, 0);
    }

    return 0;
}
