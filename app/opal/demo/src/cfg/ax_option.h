/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_OPTION_H_
#define _AX_OPTION_H_

#include "ax_global_type.h"

AX_BOOL SetCfgPath(AX_CHAR* pPath);

AX_F32 GetJencOutBuffRatio();
AX_U32 GetAencOutFrmSize();
AX_U32 GetWebJencRingBufCount();
AX_U32 GetWebMjencRingBufCount();
AX_U32 GetWebEventsRingBufCount();
AX_U32 GetWebAencRingBufCount();
AX_U32 GetRTSPMaxFrmSize();
AX_U32 GetRTSPRingBufCount();
AX_U32 GetSnapShotQpLevel();
AX_BOOL IsEnableMp4Record();
AX_U32 GetMp4SavedPath(AX_CHAR* szPath, AX_U32 nLen);
AX_U32 GetMp4FileSize();
AX_U32 GetMp4FileCount();
AX_BOOL GetMp4LoopSet();
AX_BOOL IsEnableAudio();
AX_BOOL IsEnableDIS();
AX_U8 GetDISDelayFrameNum();
AX_BOOL GetDISMotionShare();
AX_BOOL GetDISMotionEst();
AX_U32 GetAudioEncoderType();
AX_BOOL GetInterpolationResolution(AX_U32* nWidth, AX_U32* nHeight);
AX_BOOL IsEnableWebServerStatusCheck();
AX_U32 GetDetectAlgoType();
AX_BOOL IsEnableBodyAeRoi();
AX_BOOL IsEnableVehicleAeRoi();
AX_U32 GetSensorMode();
AX_BOOL GetTuningOption(AX_U32* pPort);
AX_U32 GetVencRingBufSize(AX_U32 width, AX_U32 height);

#endif // _OPTION_HELPER_H_
