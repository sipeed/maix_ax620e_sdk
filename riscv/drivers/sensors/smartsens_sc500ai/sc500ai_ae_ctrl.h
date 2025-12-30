/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SC500AI_AE_CTRL_H__
#define __SC500AI_AE_CTRL_H__

#include "ax_base_type.h"
#include "ax_sensor_struct.h"

#define SC500AI_MAX_VTS         (0x7FFF)
#define SC500AI_MAX_RATIO       (16.0f)
#define SC500AI_MIN_RATIO       (1.0f)

#define SC500AI_MAX_AGAIN       (24.067f)
#define SC500AI_MAX_DGAIN       (31.75f)

typedef struct SENSOR_SC500AI_S {
    AX_U32 vts;
} SNSSC500AI_OBJ;

typedef struct _sc500ai_GAIN_TABLE_T_ {
    float gain;
    AX_U8 gain_in;
    AX_U8 gain_de;
} sc500ai_GAIN_TABLE_T;

typedef enum _AX_SNS_AE_REG_IDX_E_ {
    SC500AI_EXP_L_H_IDX,
    SC500AI_EXP_L_M_IDX,
    SC500AI_EXP_L_L_IDX,
    SC500AI_EXP_S_H_IDX,
    SC500AI_EXP_S_M_IDX,
    SC500AI_EXP_S_L_IDX,
    SC500AI_AGAIN_L_IDX,
    SC500AI_AGAIN_L_FINE_IDX,
    SC500AI_AGAIN_S_IDX,
    SC500AI_AGAIN_S_FINE_IDX,
    SC500AI_DGAIN_L_IDX,
    SC500AI_DGAIN_L_FINE_IDX,
    SC500AI_DGAIN_S_IDX,
    SC500AI_DGAIN_S_FINE_IDX,
    SC500AI_VTS_H_IDX,
    SC500AI_VTS_L_IDX,
    SC500AI_VTS_S_H_IDX,
    SC500AI_VTS_S_L_IDX,
} AX_SNS_AE_REG_IDX_E;


AX_S32 sc500ai_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo);
AX_S32 sc500ai_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId);

AX_S32 sc500ai_cfg_aec_param(ISP_PIPE_ID nPipeId);
AX_S32 sc500ai_update_regidx_table(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue);

AX_S32 sc500ai_get_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params);
AX_S32 sc500ai_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain);
AX_S32 sc500ai_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain);
AX_S32 sc500ai_hcglcg_ctrl(ISP_PIPE_ID nPipeId, AX_U32 nSnsHcgLcg);
AX_S32 sc500ai_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam);
AX_S32 sc500ai_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio, AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange);
AX_S32 sc500ai_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime);
AX_S32 sc500ai_sns_update_group_hold_regs(ISP_PIPE_ID nPipeId, AX_U32 nRegsIdx, AX_U8 nRegsValue);
AX_S32 sc500ai_set_slow_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps);
AX_S32 sc500ai_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam);
AX_S32 sc500ai_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps);


#endif  //end __SC500AI_AE_CTRL_H__
