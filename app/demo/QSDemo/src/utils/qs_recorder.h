/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _QSRECORDER_H__
#define _QSRECORDER_H__

#define MAX_RECORD_FILE_COUNT  (5)

#include "qs_log.h"
#include "ax_global_type.h"

AX_S32  QS_MonitorSDCardStart();
AX_S32  QS_MonitorSDCardStop();
AX_BOOL QS_IsSDCardReady();

AX_S32  QS_VideoRecorderInit(AX_S32 nCamCount, AX_U32 nVideoRingBufSize, AX_U32 nAudioRingBufSize, AX_S32 nMaxRecodFileCount);
AX_S32  QS_VideoRecorderDeinit();
AX_S32  QS_VideoRecorderStart();
AX_S32  QS_VideoRecorderStop();
AX_S32  QS_SaveVideo(AX_S32 nCamIdx, AX_U8 *pData, AX_S32 nSize, AX_U64 nPts, AX_BOOL bIFrame, AX_BOOL bFlush);
AX_S32  QS_SaveAudio(AX_S32 nCamIdx, AX_U8 *pData, AX_S32 nSize, AX_U64 nPts, AX_BOOL bFlush);
AX_S32  QS_VideoRecorderWakeup(AX_BOOL bWakeup);

AX_BOOL QS_MountSDCard();
AX_BOOL QS_CheckSDMounted();
AX_BOOL QS_SweepDiskQs(AX_S32 nRunTimes);

AX_BOOL QS_CopyResultFiles(AX_BOOL bReboot);
AX_VOID QS_StartCopyResultFiles();
AX_VOID QS_StopCopyResultFiles();
AX_S32  QS_ChangeSysLogPath(AX_S32 nRunTimes);
AX_S32  QS_GetSDRuntimePath(AX_CHAR *szPath, AX_S32 nLen, AX_S32 nRunTimes, AX_BOOL bReboot);

#endif //_QSRECORDER_H__