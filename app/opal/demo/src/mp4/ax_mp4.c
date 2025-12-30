/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <string.h>
#include "ax_mp4.h"
#include "mp4_api.h"
#include "ax_log.h"


#define LOG_TAG "MP4"

#define MP4_DEFAULT_FILE_SIZE (64)
#define MP4_MAX_FILE_SIZE (4 * 1024 - 64) // 4GiB
#define MP4_DEFAULT_RECORD_FILE_NUM (10)
#define MP4_DEFAULT_VIDEO_DEPTH (5)
#define MP4_DEFAULT_AUDIO_DEPTH (5)
#define MP4_DEFAULT_LOOP_SET AX_TRUE
#define VIDEO_DEFAULT_MAX_FRM_SIZE (700000)
#define AUDIO_DEFAULT_MAX_FRM_SIZE (10240)

static AX_VOID mp4_callback(MP4_HANDLE handle, const char *file_name, mp4_status_e status, void *user_data) {
    const AX_CHAR *status_str[MP4_STATUS_BUTT] = {"", "start", "complete", "deleted", "failure", "disk full"};
    if (user_data) {
#if defined(__LP64__)
        AX_U8 nSnsId = (AX_U8)(((AX_U64)user_data) & 0xFF);
        //AX_U8 nChnId = (AX_U8)(((AX_U64)user_data) >> 8);
#else
        AX_U8 nSnsId = (AX_U8)(((AX_U32)user_data) & 0xFF);
        //AX_U8 nChnId = (AX_U8)(((AX_U32)user_data) >> 8);
#endif
        if (status == MP4_STATUS_FAILURE || status == MP4_STATUS_DISK_FULL) {
            LOG_M_I(LOG_TAG, "sns[%d]: %s status: %s", nSnsId, file_name ? file_name : "", status_str[status]);
        } else {
            LOG_M_I(LOG_TAG, "sns[%d]: %s status: %s", nSnsId, file_name ? file_name : "", status_str[status]);
        }
    }
}

AX_S32 AX_MP4_Init(AX_MP4_HANDLE *ppHandle, AX_MP4_INFO_T *pstInfo) {
    if (!ppHandle || !pstInfo) {
        return -1;
    }

    mp4_info_t mp4_info;
    memset(&mp4_info, 0, sizeof(mp4_info));

    AX_U8 nSnsId = pstInfo->nSnsId;
    AX_U8 nChn = pstInfo->nChn;

    mp4_info.loop = (pstInfo->bLoopSet == AX_TRUE) ? true : false;
    if (pstInfo->nMaxFileInMBytes > MP4_MAX_FILE_SIZE) {
        mp4_info.max_file_size = MP4_MAX_FILE_SIZE;
    } else {
        mp4_info.max_file_size = (pstInfo->nMaxFileInMBytes > 0) ? pstInfo->nMaxFileInMBytes : MP4_DEFAULT_FILE_SIZE;
    }
    mp4_info.max_file_num = (pstInfo->nMaxFileCount > 0) ? pstInfo->nMaxFileCount : MP4_DEFAULT_RECORD_FILE_NUM;
    mp4_info.dest_path = (char *)pstInfo->strSavePath;
#if defined(__LP64__)
    mp4_info.user_data = (void*)((((AX_U64)nChn) << 8) | nSnsId);
#else
    mp4_info.user_data = (void*)((((AX_U32)nChn) << 8) | nSnsId);
#endif

    char szPrefix[64] = {0};
    sprintf(szPrefix, "SNS%d_CH%d_", nSnsId, nChn);
    mp4_info.file_name_prefix = szPrefix;

    mp4_info.sr_callback = mp4_callback;

    if (pstInfo->stVideoAttr.bEnable) {
        mp4_info.video.enable = true;
        mp4_info.video.object = (pstInfo->stVideoAttr.ePt) == PT_H264 ? MP4_OBJECT_AVC : MP4_OBJECT_HEVC;
        mp4_info.video.framerate = (int)pstInfo->stVideoAttr.nFrameRate;
        mp4_info.video.bitrate = (int)pstInfo->stVideoAttr.nBitrate;
        mp4_info.video.width = (int)pstInfo->stVideoAttr.nfrWidth;
        mp4_info.video.height = (int)pstInfo->stVideoAttr.nfrHeight;
        mp4_info.video.depth = MP4_DEFAULT_VIDEO_DEPTH;
        mp4_info.video.max_frm_size = VIDEO_DEFAULT_MAX_FRM_SIZE;
    }

    if (pstInfo->stAudioAttr.bEnable) {
        mp4_info.audio.enable = true;
        if (pstInfo->stAudioAttr.ePt == PT_G711A) {
            mp4_info.audio.object = MP4_OBJECT_ALAW;
        } else if (pstInfo->stAudioAttr.ePt == PT_G711U) {
            mp4_info.audio.object = MP4_OBJECT_ULAW;
        } else if (pstInfo->stAudioAttr.ePt == PT_AAC) {
            mp4_info.audio.object = MP4_OBJECT_AAC;
        } else {
            return AX_FALSE;
        }

        mp4_info.audio.samplerate = (int)pstInfo->stAudioAttr.nSampleRate;
        mp4_info.audio.bitrate = (int)pstInfo->stAudioAttr.nBitrate;
        mp4_info.audio.chncnt = (int)pstInfo->stAudioAttr.nChnCnt;
        mp4_info.audio.aot = (int)pstInfo->stAudioAttr.nAOT;
        mp4_info.audio.depth = MP4_DEFAULT_AUDIO_DEPTH;
        mp4_info.audio.max_frm_size = AUDIO_DEFAULT_MAX_FRM_SIZE;
    }

    *ppHandle = (AX_MP4_HANDLE)mp4_create(&mp4_info);

    if (*ppHandle) {
        return 0;
    } else {
        return -1;
    }
}

AX_S32 AX_Mp4_DeInit(AX_MP4_HANDLE pHandle) {
    if (pHandle) {
        return mp4_destroy(pHandle);
    }
    return 0;
}

AX_S32 AX_Mp4_SaveVideo(AX_MP4_HANDLE pHandle, AX_VOID *data, AX_U32 size, AX_U64 nPts, AX_BOOL bIFrame) {
    return mp4_send((MP4_HANDLE)pHandle, MP4_DATA_VIDEO, data, size, nPts, (bool)bIFrame);
}

AX_S32 AX_Mp4_SaveAudio(AX_MP4_HANDLE pHandle, AX_VOID *data, AX_U32 size, AX_U64 nPts)
{
    return mp4_send((MP4_HANDLE)pHandle, MP4_DATA_AUDIO, data, size, nPts, true);
}