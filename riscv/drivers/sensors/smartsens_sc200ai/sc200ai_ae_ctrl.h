/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SC200AI_AE_CTRL_H__
#define __SC200AI_AE_CTRL_H__

#include "ax_base_type.h"
#include "ax_sensor_struct.h"


typedef enum _AX_SNS_AE_REG_IDX_E_ {
    SC200AI_EXPLINE_LONG_H_IDX,
    SC200AI_EXPLINE_LONG_M_IDX,
    SC200AI_EXPLINE_LONG_L_IDX,
    SC200AI_EXPLINE_SHORT_H_IDX,
    SC200AI_EXPLINE_SHORT_M_IDX,
    SC200AI_EXPLINE_SHORT_L_IDX,
    SC200AI_AGAIN_LONG_H_IDX,
    SC200AI_AGAIN_LONG_L_IDX,
    SC200AI_AGAIN_SHORT_H_IDX,
    SC200AI_AGAIN_SHORT_L_IDX,
    SC200AI_DGAIN_LONG_H_IDX,
    SC200AI_DGAIN_LONG_L_IDX,
    SC200AI_DGAIN_SHORT_H_IDX,
    SC200AI_DGAIN_SHORT_L_IDX,
    SC200AI_VTS_LONG_H_IDX,
    SC200AI_VTS_LONG_L_IDX,
    SC200AI_VTS_SHORT_H_IDX,
    SC200AI_VTS_SHORT_L_IDX,
} AX_SNS_AE_REG_IDX_E;

AX_S32 sc200ai_cfg_aec_param(ISP_PIPE_ID nPipeId);

AX_S32 sc200ai_get_sensor_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params);
AX_S32 sc200ai_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain);
AX_S32 sc200ai_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain);
AX_S32 sc200ai_hcglcg_ctrl(ISP_PIPE_ID nPipeId, AX_U32 nSnsHcgLcg);

AX_S32 sc200ai_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam);
AX_S32 sc200ai_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio, AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange);
AX_S32 sc200ai_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime);

AX_S32 sc200ai_set_slow_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps);
AX_S32 sc200ai_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps);
AX_S32 sc200ai_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam);
AX_S32 sc200ai_update_regidx_table(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue);
AX_S32 sc200ai_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo);
AX_U32 sc200ai_sns_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId);


#endif  //end __SC200AI_AE_CTRL_H__
