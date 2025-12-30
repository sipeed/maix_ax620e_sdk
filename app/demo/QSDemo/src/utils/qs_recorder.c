/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <sys/statfs.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include "qs_recorder.h"
#include "qs_utils.h"
#include "AXRingFifo.h"
#include "ax_sys_api.h"
#include "audio_config.h"
#include "qsdemo.h"

#define MAX_SDCARD_RETRY_TIMES (20)
#define MAX_RECORD_SNS_COUNT   (2)
#define MAX_PATH               (256)

static const AX_S32 g_nMaxFileSize = 100*1024*1024;
static AX_S32 g_nMaxRecodFileCount = MAX_RECORD_FILE_COUNT;

static pthread_mutex_t g_mutex_wakup_record[MAX_RECORD_SNS_COUNT] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
static pthread_cond_t g_cond_wakup_record[MAX_RECORD_SNS_COUNT] = {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER};
static AX_BOOL g_bWakeupRecord[MAX_RECORD_SNS_COUNT] = {AX_TRUE, AX_TRUE};

static AX_S32 g_nCamCount = 1;
AX_BOOL g_bWorking = AX_FALSE;
AX_BOOL g_bSdMonitorWorking = AX_FALSE;
static AX_BOOL g_bRecordInited = AX_FALSE;

static AX_CHAR g_szTFCardRoot[] = "/mnt/qsdemo";

extern AX_S32 g_nRunningTimes;

pthread_t g_CopyResultTid = 0;
pthread_t g_MonitorSDTid = 0;

static AX_BOOL g_bSDCardReady = AX_FALSE;

typedef struct _RECODER_FILE_INFO {
    AX_CHAR  **szVideoFile;
    AX_CHAR  **szAudioFile;
    AX_CHAR* szBaseDir;
    AX_U32   nCurVideoFileSize;
    AX_U32   nCurAudioFileSize;
    AX_U32   nCurFileIdx;
    AX_S32   nCamID;
    AX_RINGFIFO_HANDLE hVideoFifo;
    AX_RINGFIFO_HANDLE hAudioFifo;
    pthread_t tid;
}RECODER_FILE_INFO_T;

RECODER_FILE_INFO_T  g_videoRecordFile[MAX_RECORD_SNS_COUNT];

int compare(const void* a, const void* b) {
    return strcmp((AX_CHAR*)a, (AX_CHAR*)b);
}

AX_BOOL QS_MountSDCard() {
    FILE * fp;
#if defined(__LP64__)
#if defined(AX620E_NAND)
    fp = popen("if [ -e '/dev/mmcblk0p1' ]; then mount /dev/mmcblk0p1 /mnt; fi","r");
#else
    fp = popen("if [ -e '/dev/mmcblk1p1' ]; then mount /dev/mmcblk1p1 /mnt; fi","r");
#endif
#else
    AX_BOOL b620Q = IS_AX620Q ? AX_TRUE : AX_FALSE;
    if (b620Q) {
        fp = popen("if [ -e '/dev/mmcblk0p1' ]; then mount /dev/mmcblk0p1 /mnt; fi","r");
    } else {
        fp = popen("if [ -e '/dev/mmcblk1p1' ]; then mount /dev/mmcblk1p1 /mnt; fi","r");
    }
#endif
    if(fp == NULL){
        return AX_FALSE;
    }
    pclose(fp);
    return AX_TRUE;
}

AX_BOOL QS_CheckSDMounted() {
    // /dev/mmcblk1p1 /mnt vfat rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=iso8859-1,shortname=mixed,errors=remount-ro 0 0
    FILE * fp;
    fd_set read_fds;
    struct timeval timeout;
    int fd = 0;
    int ret = 0;

    AX_CHAR buf[256];
    memset(buf, 0, sizeof(buf));
#if defined(__LP64__)
#if defined(AX620E_NAND)
    fp = popen("cat /proc/mounts | grep '/dev/mmcblk0p1 '","r");
#else
    fp = popen("cat /proc/mounts | grep '/dev/mmcblk1p1 '","r");
#endif
#else
    AX_BOOL b620Q = IS_AX620Q ? AX_TRUE : AX_FALSE;
    if (b620Q) {
        fp = popen("cat /proc/mounts | grep '/dev/mmcblk0p1 '","r");
    } else {
        fp = popen("cat /proc/mounts | grep '/dev/mmcblk1p1 '","r");
    }
#endif
    if(fp == NULL){
        return AX_FALSE;
    }

    fd = fileno(fp);
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (ret > 0 && FD_ISSET(fd, &read_fds)) {
        AX_S32 len = fread(buf, 1, 255, fp);
        if (len <= 14) {
            pclose(fp);
            return AX_FALSE;
        }
        pclose(fp);
        return AX_TRUE;
    } else {
        pclose(fp);
        return AX_FALSE;
    }
}

static AX_VOID *monitor_sd_thread_func(AX_VOID *param) {

    prctl(PR_SET_NAME, "qs_sdmonitor");
#if defined(__LP64__)
#else
    AX_BOOL b620Q = IS_AX620Q ? AX_TRUE : AX_FALSE;
#endif
    AX_S32  nLastStatus = -1;
    AX_S32  nCheckRetry = 0;
    struct stat buffer;

    while (g_bSdMonitorWorking) {
        AX_S32 nDevExist = 0;

#if defined(__LP64__)
#if defined(AX620E_NAND)
        nDevExist = stat("/dev/mmcblk0p1", &buffer) == 0 ? 1 : 0;
#else
        nDevExist = stat("/dev/mmcblk1p1", &buffer) == 0 ? 1 : 0;
#endif
#else
        if (b620Q) {
            nDevExist = stat("/dev/mmcblk0p1", &buffer) == 0 ? 1 : 0;
        } else {
            nDevExist = stat("/dev/mmcblk1p1", &buffer) == 0 ? 1 : 0;
        }
#endif
        if (nLastStatus != nDevExist) {
            if (nDevExist) {
                if (!QS_CheckSDMounted()) {
                    // mount
                    QS_MountSDCard();
                }

                if (QS_CheckSDMounted()) {
                    g_bSDCardReady = AX_TRUE;

                    ALOGI("sd card mounted");
                } else {
                    ALOGI("sd card mounted failed");
                }
            } else {
                g_bSDCardReady = AX_FALSE;

                if (nLastStatus == -1) {
                    // no sd card
                } else {
                    // umount
                    QS_RunCmd("umount -l /mnt", NULL, 0);

                    ALOGI("sd card removed");
                }
            }
            nLastStatus = nDevExist;

            if (nDevExist != (AX_S32)QS_CheckSDMounted()) {
                nCheckRetry = MAX_SDCARD_RETRY_TIMES;
            } else {
                nCheckRetry = 0;
            }
        } else if (nCheckRetry > 0) {
            ALOGI("sd card retry(%d)", MAX_SDCARD_RETRY_TIMES - nCheckRetry + 1);

            if (nDevExist && !QS_CheckSDMounted()) {
                // mount
                QS_MountSDCard();

                if (QS_CheckSDMounted()) {
                    g_bSDCardReady = AX_TRUE;

                    ALOGI("sd card mounted");
                }
            } else if (!nDevExist && QS_CheckSDMounted()) {
                // umount
                QS_RunCmd("umount -l /mnt", NULL, 0);

                ALOGI("sd card removed");
            }

            if (nDevExist == (AX_S32)QS_CheckSDMounted()) {
                nCheckRetry = 0;
            } else {
                nCheckRetry --;
            }
        }

        MSSleep(200);
    }

    return NULL;
}

AX_S32 QS_MonitorSDCardStart() {
    g_bSdMonitorWorking = AX_TRUE;

    pthread_create(&g_MonitorSDTid, NULL, monitor_sd_thread_func, NULL);
    if (g_MonitorSDTid != 0) {
        return 0;
    } else {
        return -1;
    }
}

AX_S32  QS_MonitorSDCardStop() {
    ALOGI("QS_MonitorSDCardStop ++");
    g_bSdMonitorWorking = AX_FALSE;

    if (g_MonitorSDTid != 0) {
        pthread_join(g_MonitorSDTid, NULL);
    }
    ALOGI("QS_MonitorSDCardStop --");
    return 0;
}

AX_BOOL QS_IsSDCardReady() {
    return g_bSDCardReady;
}

AX_BOOL QS_CheckDisk(AX_U32 nMinLeftSize) {
    struct statfs diskInfo;
    if (statfs("/mnt/qsdemo", &diskInfo) == -1)  {
        return AX_TRUE;
    }
    AX_U64 blocksize = diskInfo.f_bsize;
    AX_U64 availableDisk = diskInfo.f_bavail * blocksize;
    AX_U64 freeDisk = diskInfo.f_bfree * blocksize;
    AX_U64 totalDisk = diskInfo.f_blocks * blocksize;
    ALOGI("QS_CheckDisk: current available: %llu MB, free: %llu MB, totoal: %llu MB", availableDisk>>20, freeDisk>>20, totalDisk>>20);
    if (availableDisk < nMinLeftSize) {
        return AX_FALSE;
    }
    return AX_TRUE;
}

AX_BOOL QS_CheckAndSweepDisk(AX_S32 nCamId) {
    AX_CHAR command[256] = {0};
    AX_S32 i = 0;
    AX_S32 idx = 0;
    AX_S32 nSize = 0;
    AX_S32 nCurIdx = g_videoRecordFile[nCamId].nCurFileIdx;
    if (nCurIdx > g_nMaxRecodFileCount ) {
        i = nCurIdx - g_nMaxRecodFileCount - 1;
    }

    AX_U32 nMinLeftSize = g_nMaxFileSize*MAX_RECORD_SNS_COUNT;

    do {
        if (QS_CheckDisk(nMinLeftSize)) {
            return AX_TRUE;
        }

        for (; i <= nCurIdx; i++) {
            AX_BOOL bSwept = AX_FALSE;
            idx = i % g_nMaxRecodFileCount;
            if (strlen(g_videoRecordFile[nCamId].szVideoFile[idx])) {
                nSize = sprintf(command, "rm -f %s", g_videoRecordFile[nCamId].szVideoFile[idx]);
                command[nSize] = 0;
                QS_RunCmd(command, NULL, 0);
                g_videoRecordFile[nCamId].szVideoFile[idx][0] = 0;
                bSwept = AX_TRUE;
            }
#ifdef QSDEMO_AUDIO_SUPPORT
            if (strlen(g_videoRecordFile[nCamId].szAudioFile[idx])) {
                nSize = sprintf(command, "rm -f %s", g_videoRecordFile[nCamId].szAudioFile[idx]);
                command[nSize] = 0;
                QS_RunCmd(command, NULL, 0);
                g_videoRecordFile[nCamId].szAudioFile[idx][0] = 0;
                bSwept = AX_TRUE;
            }
#endif

            if (bSwept) {
                break;
            }
        }
        usleep(10000);
    } while(g_bWorking);

    return AX_FALSE;
}

AX_BOOL QS_SweepDiskQs(AX_S32 nRunTimes) {
    AX_CHAR command[300] = {0};
    AX_U32 nMinLeftSize = 100*1024*1024;
    AX_S32 nCount = 0;

    if (QS_CheckDisk(nMinLeftSize)) {
        return AX_TRUE;
    }

    ALOGI("QS_SweepDiskQs: clear result folder ++");
    nCount = sprintf(command, "rm -rf /mnt/qsdemo/qs");
    if (nCount > 0 && nCount < 300) {
        command[nCount] = 0;
        QS_RunCmd(command, NULL, 0);
    }
    ALOGI("QS_SweepDiskQs: clear result folder --");

    return AX_TRUE;
}

AX_BOOL QS_ListFile(AX_S32 nCamIdx) {
    DIR *dp;
    struct dirent *dirp;
    AX_CHAR command[300] = {0};
    AX_S32 nSize = 0;
    AX_S32 i = 0;

    g_videoRecordFile[nCamIdx].nCurFileIdx = 0;
    g_videoRecordFile[nCamIdx].nCurVideoFileSize = 0;
    g_videoRecordFile[nCamIdx].nCurFileIdx = 0;
    for (i = 0; i < g_nMaxRecodFileCount; i++) {
        memset(g_videoRecordFile[nCamIdx].szVideoFile[i], 0, MAX_PATH * sizeof(char));
        memset(g_videoRecordFile[nCamIdx].szAudioFile[i], 0, MAX_PATH * sizeof(char));
    }

    if ((dp = opendir(g_videoRecordFile[nCamIdx].szBaseDir)) == NULL) {
        ALOGE("can't open %s", g_videoRecordFile[nCamIdx].szBaseDir);
        return -1;
    }
    AX_U32 nCurVideoFileIdx = 0;
#ifdef QSDEMO_AUDIO_SUPPORT
    AX_U32  nCurAudioFileIdx = 0;
#endif
    while((dirp = readdir(dp)) != NULL) {
        if (dirp->d_type == 8) {
            if (strstr(dirp->d_name, ".264") || strstr(dirp->d_name, ".265")) {
                if (nCurVideoFileIdx < g_nMaxRecodFileCount) {
                    sprintf(g_videoRecordFile[nCamIdx].szVideoFile[nCurVideoFileIdx], "%s/%s",g_videoRecordFile[nCamIdx].szBaseDir, dirp->d_name);
                    nCurVideoFileIdx ++;
                } else{
                    // delete video file
                    nSize = sprintf(command, "rm -f %s/%s", g_videoRecordFile[nCamIdx].szBaseDir, dirp->d_name);
                    command[nSize] = 0;
                    QS_RunCmd(command, NULL, 0);
                }
            } else if (strstr(dirp->d_name, ".aac") || strstr(dirp->d_name, ".g711") || strstr(dirp->d_name, ".pcm")) {
#ifdef QSDEMO_AUDIO_SUPPORT
                if (nCurAudioFileIdx < g_nMaxRecodFileCount) {
                    sprintf(g_videoRecordFile[nCamIdx].szAudioFile[nCurAudioFileIdx], "%s/%s",g_videoRecordFile[nCamIdx].szBaseDir, dirp->d_name);
                    nCurAudioFileIdx ++;
                } else{
                    // delete audio file
                    nSize = sprintf(command, "rm -f %s/%s", g_videoRecordFile[nCamIdx].szBaseDir, dirp->d_name);
                    command[nSize] = 0;
                    QS_RunCmd(command, NULL, 0);
                }
#endif
            }
        }
    }
    closedir(dp);

#ifdef QSDEMO_AUDIO_SUPPORT
    SAMPLE_AUDIO_ENCODER_INFO_T stEncoderInfo;
    AX_S32 s32Ret = COMMON_AUDIO_GetEncoderInfo(&stEncoderInfo);

    if (!s32Ret
        && stEncoderInfo.bEnable) {
        // video & audio file not equal, remove all file
        if (nCurVideoFileIdx != nCurAudioFileIdx) {
            ALOGD("sns[%d] delete all files, as video file[%d] != audio file[%d]", nCamIdx, nCurVideoFileIdx, nCurAudioFileIdx);

            AX_S32 nSize = sprintf(command, "rm %s/* -rf", g_videoRecordFile[nCamIdx].szBaseDir);
            command[nSize] = 0;
            QS_RunCmd(command, NULL, 0);
            QS_RunCmd("sync", NULL, 0);

            nCurVideoFileIdx = 0;
            nCurAudioFileIdx = 0;

            for (i = 0; i < g_nMaxRecodFileCount; i++) {
                memset(g_videoRecordFile[nCamIdx].szVideoFile[i], 0, MAX_PATH * sizeof(char));
                memset(g_videoRecordFile[nCamIdx].szAudioFile[i], 0, MAX_PATH * sizeof(char));
            }
        }
    } else if (nCurAudioFileIdx != 0) {
        ALOGD("sns[%d] delete all audio files, as audio disabled", nCamIdx);

        // remove audio file
        AX_S32 nSize = sprintf(command, "rm %s/*.aac -rf", g_videoRecordFile[nCamIdx].szBaseDir);
        command[nSize] = 0;
        QS_RunCmd(command, NULL, 0);
        nSize = sprintf(command, "rm %s/*.g711* -rf", g_videoRecordFile[nCamIdx].szBaseDir);
        command[nSize] = 0;
        QS_RunCmd(command, NULL, 0);
        nSize = sprintf(command, "rm %s/*.pcm -rf", g_videoRecordFile[nCamIdx].szBaseDir);
        command[nSize] = 0;
        QS_RunCmd(command, NULL, 0);
        QS_RunCmd("sync", NULL, 0);

        nCurAudioFileIdx = 0;
        for (i = 0; i < g_nMaxRecodFileCount; i++) {
            memset(g_videoRecordFile[nCamIdx].szAudioFile[i], 0, MAX_PATH * sizeof(char));
        }
    }
#endif

    g_videoRecordFile[nCamIdx].nCurFileIdx = nCurVideoFileIdx;

    if (nCurVideoFileIdx > 0) {
        qsort(g_videoRecordFile[nCamIdx].szVideoFile, nCurVideoFileIdx, sizeof(char*), compare);
    }

#ifdef QSDEMO_AUDIO_SUPPORT
    if (nCurAudioFileIdx > 0) {
        qsort(g_videoRecordFile[nCamIdx].szAudioFile, nCurAudioFileIdx, sizeof(char*), compare);
    }
#endif

    return 0;
}

static AX_U32 QS_GetFileName(AX_S32 nCamIdx) {
    AX_CHAR command[256] = {0};
    AX_S32 nSize = 0;
    AX_U32 nIdx = g_videoRecordFile[nCamIdx].nCurFileIdx % g_nMaxRecodFileCount;
    time_t t;
    struct tm tm;

    t = time(NULL);
    localtime_r(&t, &tm);

    if (strlen(g_videoRecordFile[nCamIdx].szVideoFile[nIdx]) != 0) {
        nSize = sprintf(command, "rm -f %s", g_videoRecordFile[nCamIdx].szVideoFile[nIdx]);
        command[nSize] = 0;
        QS_RunCmd(command, NULL, 0);
        ALOGI("sns[%d] delete video file: %s", nCamIdx, g_videoRecordFile[nCamIdx].szVideoFile[nIdx]);
    }

    g_videoRecordFile[nCamIdx].nCurVideoFileSize = 0;

    nSize = sprintf(g_videoRecordFile[nCamIdx].szVideoFile[nIdx], "%s/%04d-%02d-%02d-%02d%02d%02d.264",
            g_videoRecordFile[nCamIdx].szBaseDir,
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    g_videoRecordFile[nCamIdx].szVideoFile[nIdx][nSize] = 0;

#ifdef QSDEMO_AUDIO_SUPPORT
    if (strlen(g_videoRecordFile[nCamIdx].szAudioFile[nIdx]) != 0) {
        nSize = sprintf(command, "rm -f %s", g_videoRecordFile[nCamIdx].szAudioFile[nIdx]);
        command[nSize] = 0;
        QS_RunCmd(command, NULL, 0);
        ALOGI("sns[%d] delete audio file: %s", nCamIdx, g_videoRecordFile[nCamIdx].szAudioFile[nIdx]);
    }

    g_videoRecordFile[nCamIdx].nCurAudioFileSize = 0;

    const AX_CHAR* audio_ext = NULL;
    SAMPLE_AUDIO_ENCODER_INFO_T stEncoderInfo;
    AX_S32 s32Ret = COMMON_AUDIO_GetEncoderInfo(&stEncoderInfo);

    if (!s32Ret
        && stEncoderInfo.bEnable) {
        if (stEncoderInfo.ePt == PT_G711A) {
            audio_ext = "g711a";
        } else if (stEncoderInfo.ePt == PT_G711U) {
            audio_ext = "g711u";
        } else if (stEncoderInfo.ePt == PT_AAC) {
            audio_ext = "aac";
        } else {
            audio_ext = "pcm";
        }

        nSize = sprintf(g_videoRecordFile[nCamIdx].szAudioFile[nIdx], "%s/%04d-%02d-%02d-%02d%02d%02d.%s",
                g_videoRecordFile[nCamIdx].szBaseDir,
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, audio_ext);

        g_videoRecordFile[nCamIdx].szAudioFile[nIdx][nSize] = 0;
    } else {
        memset(g_videoRecordFile[nCamIdx].szAudioFile[nIdx], 0x00, MAX_PATH * sizeof(char));
    }
#endif

    return nIdx;
}

static AX_VOID *record_thread_func(AX_VOID *param) {

    RECODER_FILE_INFO_T * pRecFileInfo = (RECODER_FILE_INFO_T *)param;
    AX_RINGFIFO_ELEMENT_T di;
    FILE *f_video = NULL;
    FILE *f_audio = NULL;
    AX_U32 len = 0;
    AX_U32 wlen = 0;
    AX_CHAR command[256] = {0};
    AX_CHAR name[64] = {0};
    sprintf(name, "qs_record%d", pRecFileInfo->nCamID);

    prctl(PR_SET_NAME, name);

    // create dir
    AX_S32 nSize = sprintf(command, "mkdir -p %s", g_videoRecordFile[pRecFileInfo->nCamID].szBaseDir);
    command[nSize] = 0;

wait_sd:
    do {
        if (QS_IsSDCardReady()) {
            break;
        }
        usleep(100000);
    } while (g_bWorking);

    if (!g_bWorking) {
        return NULL;
    }
    __sync_synchronize();

    do {
        QS_RunCmd(command, NULL, 0);
        QS_RunCmd("sync", NULL, 0);
        if (access(g_videoRecordFile[pRecFileInfo->nCamID].szBaseDir, F_OK) == 0) {
            ALOGI("sns[%d] create dir %s success",pRecFileInfo->nCamID, g_videoRecordFile[pRecFileInfo->nCamID].szBaseDir);
            break;
        } else {
            goto wait_sd;
        }
    } while (0);

    __sync_synchronize();
    if (!g_bWorking) {
        return NULL;
    }

    QS_ListFile(pRecFileInfo->nCamID);

    QS_CheckAndSweepDisk(pRecFileInfo->nCamID);

    AX_U32 nIdx = QS_GetFileName(pRecFileInfo->nCamID);
    f_video = fopen(g_videoRecordFile[pRecFileInfo->nCamID].szVideoFile[nIdx], "wb");
    ALOGI("sns[%d] create video file: %s, ret=%d", pRecFileInfo->nCamID, g_videoRecordFile[pRecFileInfo->nCamID].szVideoFile[nIdx], errno);

    if (!f_video) {
        goto wait_sd;
    }

#ifdef QSDEMO_AUDIO_SUPPORT
    if (strlen(g_videoRecordFile[pRecFileInfo->nCamID].szAudioFile[nIdx]) != 0) {
        f_audio = fopen(g_videoRecordFile[pRecFileInfo->nCamID].szAudioFile[nIdx], "wb");
        ALOGI("sns[%d] create audio file: %s, ret=%d", pRecFileInfo->nCamID, g_videoRecordFile[pRecFileInfo->nCamID].szAudioFile[nIdx], errno);

        if (!f_audio) {
            fclose(f_video);
            f_video = NULL;

            goto wait_sd;
        }
    }
#endif

#define CHECK_SDCARD_READY() do { \
            if (!QS_IsSDCardReady()) { \
                if (f_video) { \
                    fclose(f_video); \
                    f_video = NULL; \
                } \
                if (f_audio) { \
                    fclose(f_audio); \
                    f_audio = NULL; \
                } \
                goto wait_sd; \
            } \
        } while(0)

    while (g_bWorking || AX_RingFifo_Size(pRecFileInfo->hVideoFifo) > 0 || AX_RingFifo_Size(pRecFileInfo->hAudioFifo) > 0) {
        if (g_bWorking
            && AX_RingFifo_Size(pRecFileInfo->hVideoFifo) < (AX_RingFifo_Capacity(pRecFileInfo->hVideoFifo) / 2)
            && AX_RingFifo_Size(pRecFileInfo->hAudioFifo) < (AX_RingFifo_Capacity(pRecFileInfo->hAudioFifo) / 2)) {
            usleep(100000);
            continue;
        }
        CHECK_SDCARD_READY();

        //wait for wakeup
        pthread_mutex_lock(&g_mutex_wakup_record[pRecFileInfo->nCamID]);
        while (!g_bWakeupRecord[pRecFileInfo->nCamID]) {
            pthread_cond_wait(&g_cond_wakup_record[pRecFileInfo->nCamID], &g_mutex_wakup_record[pRecFileInfo->nCamID]);
        }
        pthread_mutex_unlock(&g_mutex_wakup_record[pRecFileInfo->nCamID]);
        update_sleep_condition(pRecFileInfo->nCamID == 0 ? CONDITION_MASK_RECORD0 : CONDITION_MASK_RECORD1, false);

        ALOGI("sns[%d] write record to sd +++", pRecFileInfo->nCamID);
        do {

#ifdef QSDEMO_AUDIO_SUPPORT
            memset(&di, 0, sizeof(di));
            if (AX_RingFifo_Size(pRecFileInfo->hAudioFifo) > 0
                && AX_RingFifo_Get(pRecFileInfo->hAudioFifo, &di) == 0) {
                len = di.data[0].len + di.data[1].len;
                wlen = 0;

                if (f_audio) {
                    wlen = fwrite(di.data[0].buf, 1, di.data[0].len, f_audio);
                    if (di.data[1].len) {
                        wlen += fwrite(di.data[1].buf, 1, di.data[1].len, f_audio);
                    }
                    if (wlen != len) {
                        ALOGE("Audio sns[%d] write data to file failed, %u -> %u", pRecFileInfo->nCamID, len, wlen);
                        {
                            fflush(f_audio);
                            fsync(fileno(f_audio));
                            fclose(f_audio);
                            f_audio = NULL;
                            pRecFileInfo->nCurAudioFileSize += wlen;
                            AX_RingFifo_Pop(pRecFileInfo->hAudioFifo);

                            if (f_video) {
                                fflush(f_video);
                                fsync(fileno(f_video));
                                fclose(f_video);
                                f_video = NULL;
                            }
                            ALOGI("sns[%d] write record to sd ---", pRecFileInfo->nCamID);
                            update_sleep_condition(pRecFileInfo->nCamID == 0 ? CONDITION_MASK_RECORD0 : CONDITION_MASK_RECORD1, true);
                            goto wait_sd;
                        }
                    }
                }

                pRecFileInfo->nCurAudioFileSize += wlen;
                AX_RingFifo_Pop(pRecFileInfo->hAudioFifo);
            }
#endif

            memset(&di, 0, sizeof(di));
            if (AX_RingFifo_Get(pRecFileInfo->hVideoFifo, &di) == 0) {
                // ALOGI("[rec%d][get] pts=%llu, ifrm=%d", pRecFileInfo->nCamID, di.nPts, di.bIFrame);

                len = di.data[0].len + di.data[1].len;
                wlen = 0;
                if (di.bIFrame && (pRecFileInfo->nCurVideoFileSize + len) >= g_nMaxFileSize) {
                    if (f_video) {
                        fflush(f_video);
                        fsync(fileno(f_video));
                        fclose(f_video);
                    }

#ifdef QSDEMO_AUDIO_SUPPORT
                    if (f_audio) {
                        fflush(f_audio);
                        fsync(fileno(f_audio));
                        fclose(f_audio);
                    }
#endif
                    g_videoRecordFile[pRecFileInfo->nCamID].nCurFileIdx ++;

                    QS_CheckAndSweepDisk(pRecFileInfo->nCamID);

                    nIdx = QS_GetFileName(pRecFileInfo->nCamID);
                    f_video = fopen(g_videoRecordFile[pRecFileInfo->nCamID].szVideoFile[nIdx], "wb");
                    ALOGI("sns[%d] create video file: %s", pRecFileInfo->nCamID, g_videoRecordFile[pRecFileInfo->nCamID].szVideoFile[nIdx]);

                    if (!f_video) {
                        ALOGI("sns[%d] write record to sd ---", pRecFileInfo->nCamID);
                        update_sleep_condition(pRecFileInfo->nCamID == 0 ? CONDITION_MASK_RECORD0 : CONDITION_MASK_RECORD1, true);
                        goto wait_sd;
                    }

#ifdef QSDEMO_AUDIO_SUPPORT
                    if (strlen(g_videoRecordFile[pRecFileInfo->nCamID].szAudioFile[nIdx]) != 0) {
                        f_audio = fopen(g_videoRecordFile[pRecFileInfo->nCamID].szAudioFile[nIdx], "wb");
                        ALOGI("sns[%d] create audio file: %s", pRecFileInfo->nCamID, g_videoRecordFile[pRecFileInfo->nCamID].szAudioFile[nIdx]);

                        if (!f_audio) {
                            fclose(f_video);
                            f_video = NULL;
                            ALOGI("sns[%d] write record to sd ---", pRecFileInfo->nCamID);
                            update_sleep_condition(pRecFileInfo->nCamID == 0 ? CONDITION_MASK_RECORD0 : CONDITION_MASK_RECORD1, true);
                            goto wait_sd;
                        }
                    }
#endif
                }

                if (pRecFileInfo->nCurVideoFileSize == 0 && !di.bIFrame) {
                    AX_RingFifo_Pop(pRecFileInfo->hVideoFifo);
                    continue;
                }

                if (f_video) {
                    wlen = fwrite(di.data[0].buf, 1, di.data[0].len, f_video);
                    if (di.data[1].len) {
                        wlen += fwrite(di.data[1].buf, 1, di.data[1].len, f_video);
                    }
                    if (wlen != len) {
                        ALOGE("Video sns[%d] write data to file failed, %u -> %u", pRecFileInfo->nCamID, len, wlen);
                        {
                            fflush(f_video);
                            fsync(fileno(f_video));
                            fclose(f_video);
                            f_video = NULL;
                            pRecFileInfo->nCurVideoFileSize += wlen;
                            AX_RingFifo_Pop(pRecFileInfo->hVideoFifo);
#ifdef QSDEMO_AUDIO_SUPPORT
                            if (f_audio) {
                                fflush(f_audio);
                                fsync(fileno(f_audio));
                                fclose(f_audio);
                                f_audio = NULL;
                            }
#endif
                            ALOGI("sns[%d] write record to sd ---", pRecFileInfo->nCamID);
                            update_sleep_condition(pRecFileInfo->nCamID == 0 ? CONDITION_MASK_RECORD0 : CONDITION_MASK_RECORD1, true);
                            goto wait_sd;
                        }
                    }
                }

                pRecFileInfo->nCurVideoFileSize += wlen;
                AX_RingFifo_Pop(pRecFileInfo->hVideoFifo);
            } else {
                break;
            }
        } while(g_bWorking);

        fflush(f_video);
        fsync(fileno(f_video));
#ifdef QSDEMO_AUDIO_SUPPORT
        if (f_audio) {
            fflush(f_audio);
            fsync(fileno(f_audio));
        }
#endif
        ALOGI("sns[%d] write record to sd ---", pRecFileInfo->nCamID);
        update_sleep_condition(pRecFileInfo->nCamID == 0 ? CONDITION_MASK_RECORD0 : CONDITION_MASK_RECORD1, true);

    }

    if (f_video) {
        fflush(f_video);
        fsync(fileno(f_video));
        fclose(f_video);
    }
    if (f_audio) {
        fflush(f_audio);
        fsync(fileno(f_audio));
        fclose(f_audio);
    }

    return NULL;
}

AX_S32 QS_VideoRecorderInit(AX_S32 nCamCount, AX_U32 nVideoRingBufSize, AX_U32 nAudioRingBufSize, AX_S32 nMaxRecodFileCount)
{
    AX_S32 i = 0;
    AX_S32 j = 0;
    AX_CHAR szName[64] = {0};
    g_nCamCount = nCamCount;
    g_nMaxRecodFileCount = nMaxRecodFileCount;

    ALOGI("max record file count %d", g_nMaxRecodFileCount);

    for (i = 0; i < nCamCount; i++) {
        g_videoRecordFile[i].nCurFileIdx = 0;
        g_videoRecordFile[i].nCamID = i;
        g_videoRecordFile[i].nCurVideoFileSize = 0;
        g_videoRecordFile[i].szBaseDir = (i == 0) ? "/mnt/qsdemo/aov/sns0_vr" : "/mnt/qsdemo/aov/sns1_vr";
        g_videoRecordFile[i].nCurFileIdx = 0;
        g_videoRecordFile[i].tid = 0;
        g_videoRecordFile[i].szVideoFile = (char**)malloc(g_nMaxRecodFileCount*sizeof(char*));
        g_videoRecordFile[i].szAudioFile = (char**)malloc(g_nMaxRecodFileCount*sizeof(char*));
        for(j = 0; j < g_nMaxRecodFileCount; j++) {
            g_videoRecordFile[i].szVideoFile[j] = (char*) malloc(MAX_PATH*sizeof(char));
            g_videoRecordFile[i].szAudioFile[j] = (char*) malloc(MAX_PATH*sizeof(char));
            memset(g_videoRecordFile[i].szVideoFile[j], 0, MAX_PATH*sizeof(char));
            memset(g_videoRecordFile[i].szAudioFile[j], 0, MAX_PATH*sizeof(char));
        }
    }

    for (i = 0; i < nCamCount; i++) {
        sprintf(szName, "VRING%d", i);
        if (AX_RingFifo_Init(&g_videoRecordFile[i].hVideoFifo, nVideoRingBufSize, szName) != 0) {
            ALOGE("create video fifo failed");
            return -1;
        }
#ifdef QSDEMO_AUDIO_SUPPORT
        sprintf(szName, "ARING%d", i);
        if (AX_RingFifo_Init(&g_videoRecordFile[i].hAudioFifo, nAudioRingBufSize, szName) != 0) {
            ALOGE("create audio fifo failed");
            return -1;
        }
#endif
    }

    g_bRecordInited = AX_TRUE;
    return 0;
}

AX_S32 QS_VideoRecorderDeinit()
{
    ALOGI("QS_VideoRecorderDeinit ++");
    AX_S32 i = 0;
    AX_S32 j = 0;
    if (!g_bRecordInited) {
        return 0;
    }

    g_bRecordInited = AX_FALSE;

    for (i = 0; i < g_nCamCount; i++) {
        if (g_videoRecordFile[i].hVideoFifo) {
            AX_RingFifo_Deinit(g_videoRecordFile[i].hVideoFifo);
            g_videoRecordFile[i].hVideoFifo = NULL;
        }

        if (g_videoRecordFile[i].hAudioFifo) {
            AX_RingFifo_Deinit(g_videoRecordFile[i].hAudioFifo);
            g_videoRecordFile[i].hAudioFifo = NULL;
        }

        for(j = 0; j < g_nMaxRecodFileCount; j++) {
            free(g_videoRecordFile[i].szVideoFile[j]);
            free(g_videoRecordFile[i].szAudioFile[j]);
        }
        free(g_videoRecordFile[i].szVideoFile);
        free(g_videoRecordFile[i].szAudioFile);
    }
    ALOGI("QS_VideoRecorderDeinit --");
    return 0;
}

AX_S32 QS_VideoRecorderStart() {
    AX_S32 i = 0;

    if (!g_bRecordInited || g_bWorking) {
        return 0;
    }
    g_bWorking = AX_TRUE;

    for (i = 0; i < g_nCamCount; i++) {
        // create thread to save data
        pthread_create(&g_videoRecordFile[i].tid, NULL, record_thread_func, &g_videoRecordFile[i]);
    }
    return 0;
}

AX_S32 QS_VideoRecorderStop() {
    ALOGI("QS_VideoRecorderStop ++");
    AX_S32 i = 0;
    if (!g_bRecordInited || !g_bWorking) {
        return 0;
    }
    g_bWorking = AX_FALSE;
    for (i = 0; i < g_nCamCount; i++) {
        // join thread
        if (g_videoRecordFile[i].tid != 0) {
            pthread_join(g_videoRecordFile[i].tid, NULL);
        }
    }
    ALOGI("QS_VideoRecorderStop --");
    return 0;
}

AX_S32 QS_SaveVideo(AX_S32 nCamIdx, AX_U8 *pData, AX_S32 nSize, AX_U64 nPts, AX_BOOL bIFrame, AX_BOOL bFlush)
{
    AX_S32 ret = 0;
    if (nCamIdx >= g_nCamCount) {
        return 0;
    }

    if (!g_bRecordInited) {
        //ALOGE("recorder is not inited!");
        return 0;
    }

    ret = AX_RingFifo_Put(g_videoRecordFile[nCamIdx].hVideoFifo, pData, nSize, nPts, bIFrame);
    // if (ret == 0) {
    //     ALOGI("[rec%d][put] pts=%llu, ifrm=%d", nCamIdx, nPts, bIFrame);
    // } else {
    //     ALOGI("[rec%d][put] pts=%llu, ifrm=%d, ring is full", nCamIdx, nPts, bIFrame);
    // }

    if (bFlush) {
        while (AX_RingFifo_Size(g_videoRecordFile[nCamIdx].hVideoFifo) != 0) {
            usleep(10000);
        }
    }

    return ret;
}

AX_S32 QS_SaveAudio(AX_S32 nCamIdx, AX_U8 *pData, AX_S32 nSize, AX_U64 nPts, AX_BOOL bFlush)
{
    AX_S32 ret = 0;
    if (nCamIdx >= g_nCamCount) {
        return 0;
    }

    if (!g_bRecordInited) {
        //ALOGE("recorder is not inited!");
        return 0;
    }

    ret = AX_RingFifo_Put(g_videoRecordFile[nCamIdx].hAudioFifo, pData, nSize, nPts, AX_TRUE);

    if (bFlush) {
        while (AX_RingFifo_Size(g_videoRecordFile[nCamIdx].hAudioFifo) != 0) {
            usleep(10000);
        }
    }

    return ret;
}

AX_S32  QS_VideoRecorderWakeup(AX_BOOL bWakeup) {
    for (AX_S32 i = 0; i < g_nCamCount; i++) {
        pthread_mutex_lock(&g_mutex_wakup_record[i]);
        g_bWakeupRecord[i] = bWakeup;
        if (bWakeup) {
            pthread_cond_signal(&g_cond_wakup_record[i]);
        }
        pthread_mutex_unlock(&g_mutex_wakup_record[i]);
    }

    return 0;
}

AX_BOOL QS_CopyResultFiles(AX_BOOL bReboot) {

    ALOGI("QS_CopyResultFiles ++");

    if (!QS_IsSDCardReady()) {
        ALOGI("QS_CopyResultFiles --, no sd card");
        return -1;
    }
    AX_CHAR command[MAX_PATH] = {0};
    AX_CHAR destRootPath[128] = {0};
    AX_S32 nSize = 0;
    if (bReboot) {
        nSize = sprintf(destRootPath, "%s/qs/%d", g_szTFCardRoot, g_nRunningTimes);
    } else {
        nSize = sprintf(destRootPath, "%s/aov/log/%d", g_szTFCardRoot, g_nRunningTimes);
    }
    destRootPath[nSize] = 0;

#define RUN_CMD(s) do {\
        if (s > 0 && s < MAX_PATH) {\
            command[s] = 0;\
            QS_RunCmd(command, NULL, 0);\
        }\
    } while(0)

    nSize = sprintf(command, "mkdir -p %s", g_szTFCardRoot);
    RUN_CMD(nSize);

    if (bReboot) {
        if (g_nRunningTimes == 1) {
            // clear sd dir for 1st run time
            nSize = sprintf(command, "rm -rf %s", g_szTFCardRoot);
            RUN_CMD(nSize);
        } else {
            // check sd space and remove old runtime dir if space is not enough
            // QS_SweepDiskQs(g_nRunningTimes);
        }
    }

    // mkdir output
    nSize = sprintf(command, "mkdir -p %s", destRootPath);
    RUN_CMD(nSize);

    // log
    if (IS_AX620Q) {
        nSize = sprintf(command, "cp -Lrf /opt/data/AXSyslog %s", destRootPath);
        RUN_CMD(nSize);
    } else {
        nSize = sprintf(command, "cp -Lrf /customer/data/AXSyslog %s", destRootPath);
        RUN_CMD(nSize);
        // 630C, delete log, because log will increase and copying time will increase.
        nSize = sprintf(command, "/etc/init.d/S02klogd stop");
        RUN_CMD(nSize);
        nSize = sprintf(command, "rm -rf /customer/data/AXSyslog");
        RUN_CMD(nSize);
        nSize = sprintf(command, "mkdir -p /customer/data/AXSyslog/kernel");
        RUN_CMD(nSize);
        nSize = sprintf(command, "/etc/init.d/S01syslogd restart && /etc/init.d/S02klogd start");
        RUN_CMD(nSize);
    }

    if (bReboot) {
        // riscv log
        nSize = sprintf(command, "cat /proc/ax_proc/riscv/log_dump > %s/riscv.log", destRootPath);
        RUN_CMD(nSize);

        // jpeg
        nSize = sprintf(command, "if [ -e '/tmp/jenc0.jpg' ]; then cp -f /tmp/*.jpg %s; fi", destRootPath);
        RUN_CMD(nSize);

        // h264
        nSize = sprintf(command, "if [ -e '/tmp/venc0.264' ]; then cp -f /tmp/*.264 %s; fi", destRootPath);
        RUN_CMD(nSize);
    }

    // sync
    QS_RunCmd("sync", NULL, 0);

    ALOGI("copy result files to %s done", destRootPath);

    return 0;
}

AX_VOID * _CopyResultFiles(AX_VOID * para) {
    prctl(PR_SET_NAME, "qs_copy_result");
    QS_CopyResultFiles(AX_TRUE);
    return NULL;
}

AX_VOID QS_StartCopyResultFiles() {
    pthread_create(&g_CopyResultTid, NULL, _CopyResultFiles, NULL);
}

AX_VOID QS_StopCopyResultFiles() {
    ALOGI("QS_StopCopyResultFiles ++");
    if (g_CopyResultTid != 0) {
        pthread_join(g_CopyResultTid, NULL);
    }
    ALOGI("QS_StopCopyResultFiles --");
}

AX_S32  QS_ChangeSysLogPath(AX_S32 nRunTimes) {
    // mount /dev/mmcblk0p1 /mnt
    // sed -i -re "s/AX_SYSLOG_path =.*/AX_SYSLOG_path = \/mnt\/log /g" /var/log/AXSyslog/ax_syslog.conf
    // sed -i -re "s/user\.\*.*    \/var\/log\/AXSyslog\/kernel\/user.log/user\.\*          \/mnt\/log\/AXSyslog\/kernel\/user.log/g" /var/log/AXSyslog/syslog.conf
    // mkdir -p /mnt/log/AXSyslog/kernel
    // mkdir -p /mnt/log/AXSyslog/syslog
    // /etc/init.d/axklogd stop
    // /etc/init.d/axsyslogd restart
    // /etc/init.d/axklogd start &
    AX_CHAR szCmd[256] = {0};
    AX_S32 nCount = 0;
    AX_CHAR szLogPath[128] = {0};
    AX_S32 nSize = 0;

    nSize = sprintf(szLogPath, "%s/qs/%d/syslog", g_szTFCardRoot, nRunTimes);
    szLogPath[nSize] = 0;

    nCount = sprintf(szCmd, "mkdir -p %s", szLogPath);
    szCmd[nCount] = 0;
    QS_RunCmd(szCmd, NULL, 0);

    // AX_SYSLOG_filesizemax = 2     #单个Log文件大小,单位:MB。
    // AX_SYSLOG_volume = 4          #所有log占用的总磁盘空间,单位:MB。

    if (IS_AX620Q) {
        nCount = sprintf(szCmd, "sed -i -re 's|AX_SYSLOG_path =.*|AX_SYSLOG_path = %s|g' /var/log/AXSyslog/ax_syslog.conf", szLogPath);
        szCmd[nCount] = 0;
        QS_RunCmd(szCmd, NULL, 0);

        nCount = sprintf(szCmd, "sed -i -re 's|AX_SYSLOG_filesizemax =.*|AX_SYSLOG_filesizemax = 32|g' /var/log/AXSyslog/ax_syslog.conf");
        szCmd[nCount] = 0;
        QS_RunCmd(szCmd, NULL, 0);

        nCount = sprintf(szCmd, "sed -i -re 's|AX_SYSLOG_volume =.*|AX_SYSLOG_volume = 1024|g' /var/log/AXSyslog/ax_syslog.conf");
        szCmd[nCount] = 0;
        QS_RunCmd(szCmd, NULL, 0);
    } else {
        nCount = sprintf(szCmd, "sed -i -re 's|AX_SYSLOG_path =.*|AX_SYSLOG_path = %s|g' /etc/ax_syslog.conf", szLogPath);
        szCmd[nCount] = 0;
        QS_RunCmd(szCmd, NULL, 0);
    }

    nCount = sprintf(szCmd, "/etc/init.d/axsyslogd reload");
    szCmd[nCount] = 0;
    system(szCmd);

    ALOGI("syslog path changed to: %s", szLogPath);

    return 0;
}

AX_S32 QS_GetSDRuntimePath(AX_CHAR *szPath, AX_S32 nLen, AX_S32 nRunTimes, AX_BOOL bReboot) {
    if (!szPath || nLen <= 0) {
        return -1;
    }
    AX_CHAR szCmd[256] = {0};

    AX_S32 nSize = 0;
    if (bReboot) {
        nSize = snprintf(szPath, nLen - 1, "%s/qs/%d", g_szTFCardRoot, nRunTimes);
    } else {
        nSize = snprintf(szPath, nLen - 1, "%s/aov/%d", g_szTFCardRoot, nRunTimes);
    }

    szPath[nSize] = 0;

    nSize = sprintf(szCmd, "mkdir -p %s", szPath);
    szCmd[nSize] = 0;
    QS_RunCmd(szCmd, NULL, 0);

    return 0;
}
