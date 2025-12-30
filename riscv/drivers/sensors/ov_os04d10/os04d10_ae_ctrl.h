/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OS04D10_AE_CTRL_H__
#define __OS04D10_AE_CTRL_H__

#include "ax_base_type.h"
#include "ax_sensor_struct.h"

#define OS04D10_MAX_VTS         (0xFFFF)
#define OS04D10_MAX_RATIO       (32.0f)
#define OS04D10_MIN_RATIO       (1.0f)

#define OS04D10_MIN_AGAIN       (1.0f)
#define OS04D10_MAX_AGAIN       (15.5f)

#define OS04D10_MIN_DGAIN       (1.0f)
#define OS04D10_MAX_DGAIN       (32.0f)

#define OS04D10_SDR_EXP_LINE_MIN                  (1.0f)
#define OS04D10_SDR_EXP_LINE_MAX(vts)             (vts - 16)
#define OS04D10_SDR_INTEGRATION_TIME_STEP         (1.0f)

#define OS04D10_EXP_OFFSET_SDR           (0.12f) //unit:line

typedef struct SENSOR_OS04D10_S {
    AX_U32 vts;
    AX_U32 hts;
    AX_F32 sclk;
    AX_U32 setting_vts;
    AX_F32 setting_fps;
} SNSOS04D10_OBJ;

typedef struct _OS04D10_AGAIN_TABLE_T_ {
    float gain;
    AX_U8 again_in;
} OS04D10_AGAIN_TABLE_T;

typedef struct _OS04D10_DGAIN_TABLE_T_ {
    float gain;
    AX_U8 dgain_in;
    AX_U8 dgain_de;
} OS04D10_DGAIN_TABLE_T;

typedef enum _AX_SNS_AE_REG_IDX_E_ {
    OS04D10_PAGE_FLG_D2_IDX = 0,
    OS04D10_LONG_EXP_LINE_H_IDX,
    OS04D10_LONG_EXP_LINE_L_IDX,
    OS04D10_LONG_AGAIN_IDX,
    OS04D10_PAGE_FLG_D2_P5_IDX,
    OS04D10_LONG_DGAIN_H_IDX,
    OS04D10_LONG_DGAIN_L_IDX,

    OS04D10_PAGE_FLG_D2_P1_IDX,
    OS04D10_VBLANK_H_IDX,
    OS04D10_VBLANK_L_IDX,

    OS04D10_HCG_LCG_IDX,

    OS04D10_VBLANK_GAIN_EXP_EN_IDX,
    OS04D10_REG_MAX_IDX,
} AX_SNS_AE_REG_IDX_E;

AX_S32 os04d10_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo);
AX_S32 os04d10_hcglcg_ctrl(ISP_PIPE_ID nPipeId, AX_U32 nSnsHcgLcg);
AX_S32 os04d10_sns_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId);

AX_S32 os04d10_cfg_aec_param(ISP_PIPE_ID nPipeId);

AX_S32 os04d10_get_sensor_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params);
AX_S32 os04d10_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain);
AX_S32 os04d10_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain);
AX_S32 os04d10_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam);
AX_S32 os04d10_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio, AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange);
AX_S32 os04d10_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime);
AX_S32 os04d10_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam);
AX_S32 os04d10_get_fps(ISP_PIPE_ID nPipeId, AX_F32 *pFps);
AX_S32 os04d10_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps);
AX_S32 os04d10_sns_update_regidx_table(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue);


#endif  //end __OS04D10_AE_CTRL_H__
