/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "ax_base_type.h"
#include "ax_isp_common.h"

#include "isp_sensor_types.h"
#include "isp_sensor_internal.h"

#include "os04d10_reg.h"
#include "os04d10_ae_ctrl.h"

#include <rtdevice.h>

extern SNS_STATE_OBJ *g_szOs04d10Ctx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs04d10Ctx[dev])

static SNSOS04D10_OBJ sns_os04d10params[AX_VIN_MAX_PIPE_NUM];
static AX_F32 nAgainTable[SENSOR_MAX_GAIN_STEP];
static AX_F32 nDgainTable[SENSOR_MAX_GAIN_STEP];
extern AX_U8 gOs04D10SlaveAddr[AX_VIN_MAX_PIPE_NUM];

/*user config*/
static AX_F32 gFpsGear[] = {1.00, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00,
                            11.00, 12.00, 13.00, 14.00, 15.00, 16.00, 17.00, 18.00, 19.00, 20.00,
                            21.00, 22.00, 23.00, 24.00, 25.00, 26.00, 27.00, 28.00, 29.00, 30.00, 60.00
                           };

static AX_SNS_DRV_DELAY_TABLE_T gOs04d10AeRegsTable[] = {
    /* regs index */                  /* regs addr */           /*regs value*/   /*Delay Frame Num*/
    {OS04D10_PAGE_FLG_D2_IDX,         OS04D10_PAGE_FLG_D2,           0,          0},
    {OS04D10_LONG_EXP_LINE_H_IDX,     OS04D10_LONG_EXP_LINE_H,       0,          0},
    {OS04D10_LONG_EXP_LINE_L_IDX,     OS04D10_LONG_EXP_LINE_L,       0,          0},
    {OS04D10_LONG_AGAIN_IDX,          OS04D10_LONG_AGAIN,            0,          0},
    {OS04D10_PAGE_FLG_D2_P5_IDX,      OS04D10_PAGE_FLG_D2,           0,          0},
    {OS04D10_LONG_DGAIN_H_IDX,        OS04D10_LONG_DGAIN_H,          0,          0},
    {OS04D10_LONG_DGAIN_L_IDX,        OS04D10_LONG_DGAIN_L,          0,          0},

    {OS04D10_PAGE_FLG_D2_P1_IDX,      OS04D10_PAGE_FLG_D2,           0,          0},
    {OS04D10_VBLANK_H_IDX,            OS04D10_VBLANK_H,              0,          0},
    {OS04D10_VBLANK_L_IDX,            OS04D10_VBLANK_L,              0,          0},

    {OS04D10_HCG_LCG_IDX,             OS04D10_HCG_LCG,               0,          0},

    {OS04D10_VBLANK_GAIN_EXP_EN_IDX,  OS04D10_VBLANK_GAIN_EXP_EN,    0,          0},
};

const OS04D10_AGAIN_TABLE_T os04d10_again_table[] = {
    {1      ,  0x10},
    {1.0625 ,  0x11},
    {1.125  ,  0x12},
    {1.1875 ,  0x13},
    {1.25   ,  0x14},
    {1.3125 ,  0x15},
    {1.375  ,  0x16},
    {1.4375 ,  0x17},
    {1.5    ,  0x18},
    {1.5625 ,  0x19},
    {1.625  , 0x1a},
    {1.6875 , 0x1b},
    {1.75   , 0x1c},
    {1.8125 , 0x1d},
    {1.875  , 0x1e},
    {1.9375 , 0x1f},
    {2      , 0x21},
    {2.125  , 0x23},
    {2.25   , 0x25},
    {2.375  , 0x27},
    {2.5    , 0x29},
    {2.625  , 0x2b},
    {2.75   , 0x2d},
    {2.875  , 0x2f},
    {3      , 0x31},
    {3.125  , 0x33},
    {3.25   , 0x35},
    {3.375  , 0x37},
    {3.5    , 0x39},
    {3.625  , 0x3b},
    {3.75   , 0x3d},
    {3.875  , 0x3f},
    {4      , 0x43},
    {4.25   , 0x47},
    {4.5    , 0x4b},
    {4.75   , 0x4f},
    {5      , 0x53},
    {5.25   , 0x57},
    {5.5    , 0x5b},
    {5.75   , 0x5f},
    {6      , 0x63},
    {6.25   , 0x67},
    {6.5    , 0x6b},
    {6.75   , 0x6f},
    {7      , 0x73},
    {7.25   , 0x77},
    {7.5    , 0x7b},
    {7.75   , 0x7f},
    {8      , 0x87},
    {8.5    , 0x8f},
    {9      , 0x97},
    {9.5    , 0x9f},
    {10     , 0xa7},
    {10.5   , 0xaf},
    {11     , 0xb7},
    {11.5   , 0xbf},
    {12     , 0xc7},
    {12.5   , 0xcf},
    {13     , 0xd7},
    {13.5   , 0xdf},
    {14     , 0xe7},
    {14.5   , 0xef},
    {15     , 0xf7},
    {15.5   , 0xff},
};

const OS04D10_DGAIN_TABLE_T os04d10_dgain_table[] = {
    {1     ,  0x00, 0x40},
    {1.125 ,  0x00, 0x48},
    {1.25  ,  0x00, 0x50},
    {1.375 ,  0x00, 0x58},
    {1.5   ,  0x00, 0x60},
    {1.625 ,  0x00, 0x68},
    {1.75  ,  0x00, 0x70},
    {1.875 ,  0x00, 0x78},
    {2     ,  0x00, 0x80},
    {2.25  ,  0x00, 0x90},
    {2.5   , 0x00, 0xA0},
    {2.75  , 0x00, 0xB0},
    {3     , 0x00, 0xC0},
    {3.25  , 0x00, 0xD0},
    {3.5   , 0x00, 0xE0},
    {3.75  , 0x00, 0xF0},
    {4     , 0x01, 0x00},
    {4.5   , 0x01, 0x20},
    {5     , 0x01, 0x40},
    {5.5   , 0x01, 0x60},
    {6     , 0x01, 0x80},
    {6.5   , 0x01, 0xA0},
    {7     , 0x01, 0xC0},
    {7.5   , 0x01, 0xE0},
    {8     , 0x02, 0x00},
    {9     , 0x02, 0x40},
    {10    , 0x02, 0x80},
    {11    , 0x02, 0xC0},
    {12    , 0x03, 0x00},
    {13    , 0x03, 0x40},
    {14    , 0x03, 0x80},
    {15    , 0x03, 0xC0},
    {16    , 0x04, 0x00},
    {18    , 0x04, 0x80},
    {20    , 0x05, 0x00},
    {22    , 0x05, 0x80},
    {24    , 0x06, 0x00},
    {26    , 0x06, 0x80},
    {28    , 0x07, 0x00},
    {30    , 0x07, 0x80},
    {32    , 0x07, 0xFF},
};


AX_S32 os04d10_again2value(AX_F32 gain, AX_U8 *again_in, AX_F32 *gain_value)
{
    AX_U32 i;
    AX_U32 count;

    if (!again_in)
        return -1;

    count = sizeof(os04d10_again_table) / sizeof(os04d10_again_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > os04d10_again_table[i].gain) {
            continue;
        } else {
            *again_in = os04d10_again_table[i].again_in;
            *gain_value = os04d10_again_table[i].gain;
            SNS_DBG("again=%f, again_in=0x%x\n", gain, *again_in);
            return SNS_SUCCESS;
        }
    }

    return -1;
}

AX_S32 os04d10_dgain2value(AX_F32 gain, AX_U8 *dgain_in, AX_U8 *dgain_de, AX_F32 *gain_value)
{
    AX_U32 i;
    AX_U32 count;

    if (!dgain_in || !dgain_de)
        return -1;

    count = sizeof(os04d10_dgain_table) / sizeof(os04d10_dgain_table[0]);

    for (i = 0; i < count; i++) {
        if (gain > os04d10_dgain_table[i].gain) {
            continue;
        } else {
            *dgain_in = os04d10_dgain_table[i].dgain_in;
            *dgain_de = os04d10_dgain_table[i].dgain_de;
            *gain_value = os04d10_dgain_table[i].gain;
            SNS_DBG("dgain=%f, dgain_in=0x%x, dgain_de=0x%x\n", gain, *dgain_in, *dgain_de);
            return SNS_SUCCESS;
        }
    }

    return -1;
}

AX_S32 os04d10_get_gain_table(AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_U32 i;
    AX_S32 ret = 0;
    if (!params)
        return -1;

    params->nAgainTableSize = sizeof(os04d10_again_table) / sizeof(os04d10_again_table[0]);
    params->nDgainTableSize = sizeof(os04d10_dgain_table) / sizeof(os04d10_dgain_table[0]);

    for (i = 0; i < params->nAgainTableSize ; i++) {
        nAgainTable[i] = os04d10_again_table[i].gain;
        params->pAgainTable = nAgainTable;
    }

    for (i = 0; i < params->nDgainTableSize ; i++) {
        nDgainTable[i] = os04d10_dgain_table[i].gain;
        params->pDgainTable = nDgainTable;
    }

    return ret;
}

static AX_S32 os04d10_set_exp_limit(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio, AX_U32 nVts)
{
    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] = OS04D10_SDR_EXP_LINE_MIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] = OS04D10_SDR_EXP_LINE_MAX(sns_os04d10params[nPipeId].vts);
    sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeIncrement[HDR_LONG_FRAME_IDX] = OS04D10_SDR_INTEGRATION_TIME_STEP;

    SNS_DBG("pipe:%d, ratio:%f, expline_limit:%.2f-%.2f-%.2f-%.2f(%.2f-%.2f-%.2f-%.2f), line_period:%f, vts:0x%x\n",
        nPipeId, fHdrRatio,
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX],
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX],
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX],
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_MEDIUM_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_MEDIUM_FRAME_IDX] * sns_obj->ae_ctrl_param.fTimePerLine,
        sns_obj->ae_ctrl_param.fTimePerLine, nVts);

    return SNS_SUCCESS;
}

static AX_SNS_HCGLCG_MODE_E os04d10_get_dcg_mode(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    AX_U32 value = 0;
    AX_U8 data = 0;

    ret = os04d10_read_register(nPipeId, OS04D10_HCG_LCG, &value);
    if (ret) {
        SNS_ERR("read hcglcg error, ret:0x%x\n", ret);
        return AX_LCG_NOTSUPPORT_MODE;
    }

    data = value & 0xff;

    if ((data & 0x20) != 0) {
        return AX_HCG_MODE;
    } else {
        return AX_LCG_MODE;
    }

    return AX_LCG_NOTSUPPORT_MODE;
}

AX_S32 os04d10_get_params_from_setting(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    AX_U32 reg_cnt = 0;
    camera_i2c_reg_array *setting = AX_NULL;
    AX_U32 vblank = 0;
    AX_U32 hts = 0;
    AX_F32 sclk = 0;

    ret = os04d10_select_setting(nPipeId, &setting, &reg_cnt);
    if (ret) {
        return ret;
    }

    ret |= os04d10_get_vblank_from_setting(nPipeId, setting, reg_cnt, &vblank);
    ret |= os04d10_get_hts_from_setting(nPipeId, setting, reg_cnt, &hts);
    ret |= os04d10_get_sclk_from_setting(nPipeId, setting, reg_cnt, &sclk);
    if (ret) {
        return ret;
    }

    sns_os04d10params[nPipeId].vts = vblank + sns_os04d10params[nPipeId].setting_vts;
    sns_os04d10params[nPipeId].hts = hts;
    sns_os04d10params[nPipeId].sclk = sclk;

    SNS_DBG("pipe:%d, get setting params vts:0x%x hts:0x%x sclk:%f\n",
            nPipeId, sns_os04d10params[nPipeId].vts, sns_os04d10params[nPipeId].hts, sns_os04d10params[nPipeId].sclk);

    return SNS_SUCCESS;
}

AX_S32 os04d10_cfg_aec_param(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (sns_obj->eImgMode == e_OS04D10_2lane_1280x720_10bit_Linear_60fps) {
        sns_os04d10params[nPipeId].setting_vts = OS04D10_VTS_DEFAULT_2X2_BINNING;
    } else {
        sns_os04d10params[nPipeId].setting_vts = OS04D10_VTS_DEFAULT;
    }
    os04d10_get_params_from_setting(nPipeId);
    sns_obj->sns_mode_obj.nVts = sns_os04d10params[nPipeId].vts;
    sns_os04d10params[nPipeId].setting_fps = sns_obj->sns_mode_obj.fFrameRate;

    sns_obj->ae_ctrl_param.fTimePerLine = sns_os04d10params[nPipeId].hts / sns_os04d10params[nPipeId].sclk * SNS_1_SECOND_UNIT_US;

    /* sensor again  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX] = OS04D10_MIN_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX] = OS04D10_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 16;

    /* sensor dgain  limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX] = OS04D10_MIN_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX] = OS04D10_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_LONG_FRAME_IDX] = (AX_F32)1 / 1024;

    /* sensor medium again limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_MEDIUM_FRAME_IDX] = OS04D10_MIN_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_MEDIUM_FRAME_IDX] = OS04D10_MAX_AGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fAGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 16;

    /* sensor medium dgain limit*/
    sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_MEDIUM_FRAME_IDX] = OS04D10_MIN_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_MEDIUM_FRAME_IDX] = OS04D10_MAX_DGAIN;
    sns_obj->ae_ctrl_param.sns_ae_param.fDGainIncrement[HDR_MEDIUM_FRAME_IDX] = (AX_F32)1 / 1024;

    sns_obj->ae_ctrl_param.sns_ae_limit.fMinRatio = OS04D10_MIN_RATIO;
    sns_obj->ae_ctrl_param.sns_ae_limit.fMaxRatio = OS04D10_MAX_RATIO;

    os04d10_set_exp_limit(nPipeId, OS04D10_MAX_RATIO, sns_os04d10params[nPipeId].vts);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = sns_obj->sns_mode_obj.fFrameRate;

    sns_obj->ae_ctrl_param.eSnsHcgLcgMode = os04d10_get_dcg_mode(nPipeId);
    sns_obj->ae_ctrl_param.fSnsHcgLcgRatio = 3.6;
    sns_obj->ae_ctrl_param.sns_ae_param.fIntegrationTimeOffset[HDR_LONG_FRAME_IDX] = OS04D10_EXP_OFFSET_SDR;

    SNS_DBG("inttime min = %.2f, max = %.2f, line_period=%f, vts = %d \n",
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX],
            sns_obj->ae_ctrl_param.fTimePerLine, sns_os04d10params[nPipeId].vts);

    return SNS_SUCCESS;
}

AX_S32 os04d10_sns_update_regidx_table(ISP_PIPE_ID nPipeId, AX_U8 nRegIdx, AX_U8 nRegValue)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sztRegsInfo[0].sztI2cData[nRegIdx].nData = nRegValue;

    SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
        nRegIdx, sns_obj->sztRegsInfo[0].sztI2cData[nRegIdx].nRegAddr, nRegValue);

    return SNS_SUCCESS;
}

AX_S32 os04d10_get_sensor_gain_table(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_TABLE_T *params)
{
    AX_S32 result = 0;
    SNS_CHECK_PTR_VALID(params);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    result = os04d10_get_gain_table(params);
    return result;

    return SNS_SUCCESS;
}


AX_S32 os04d10_set_again(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptAGain)
{
    AX_U8 Gain_in;
    AX_S32 result = 0;
    AX_F32 gain_value = 0;
    AX_F32 nGainFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptAGain);

    /* long gain seting */
    nGainFromUser = ptAGain->fGain[HDR_LONG_FRAME_IDX];
    nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinAgain[HDR_LONG_FRAME_IDX],
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMaxAgain[HDR_LONG_FRAME_IDX]);

    result = os04d10_again2value(nGainFromUser, &Gain_in, &gain_value);
    if (result) {
        SNS_ERR("new gain match failed \n");
        return result;
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX] = gain_value;
        result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_PAGE_FLG_D2_IDX, OS04D10_PAGE_FLG_D2_P1);
        result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_LONG_AGAIN_IDX, (Gain_in & 0xFF));
        result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_VBLANK_GAIN_EXP_EN_IDX, 0x01);
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    SNS_DBG("again: long frame %f\n",
            sns_obj->ae_ctrl_param.sns_ae_param.fCurAGain[HDR_LONG_FRAME_IDX]);

    return SNS_SUCCESS;
}

AX_S32 os04d10_set_dgain(ISP_PIPE_ID nPipeId, AX_SNS_AE_GAIN_CFG_T *ptDGain)
{
    AX_U8 Gain_in;
    AX_U8 Gain_de;
    AX_S32 result = 0;
    AX_F32 gain_value = 0;
    AX_F32 nGainFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptDGain);

    /* long frame digital gain seting */
    nGainFromUser = ptDGain->fGain[HDR_LONG_FRAME_IDX];
    nGainFromUser = AXSNS_CLIP3(nGainFromUser, sns_obj->ae_ctrl_param.sns_ae_limit.fMinDgain[HDR_LONG_FRAME_IDX],
                                sns_obj->ae_ctrl_param.sns_ae_limit.fMaxDgain[HDR_LONG_FRAME_IDX]);

    result = os04d10_dgain2value(nGainFromUser, &Gain_in, &Gain_de, &gain_value);
    if (result) {
        SNS_ERR("new gain match failed \n");
        return result;
    } else {
        sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX] = gain_value;
        result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_PAGE_FLG_D2_P5_IDX, OS04D10_PAGE_FLG_D2_P5);
        result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_LONG_DGAIN_H_IDX, (Gain_in & 0xFF));
        result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_LONG_DGAIN_L_IDX, (Gain_de & 0xFF));
        result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_PAGE_FLG_D2_P1_IDX, OS04D10_PAGE_FLG_D2_P1);
        result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_VBLANK_GAIN_EXP_EN_IDX, 0x01);
        if (result != 0) {
            SNS_ERR("write hw failed %d \n", result);
            return result;
        }
    }

    SNS_DBG("dgain: long frame %f\n",
            sns_obj->ae_ctrl_param.sns_ae_param.fCurDGain[HDR_LONG_FRAME_IDX]);

    return SNS_SUCCESS;
}

AX_S32 os04d10_get_integration_time_range(ISP_PIPE_ID nPipeId, AX_F32 fHdrRatio,
        AX_SNS_AE_INT_TIME_RANGE_T *ptIntTimeRange)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTimeRange);

    ptIntTimeRange->fMaxIntegrationTime[HDR_LONG_FRAME_IDX] =
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX];
    ptIntTimeRange->fMinIntegrationTime[HDR_LONG_FRAME_IDX] =
        sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX];

    return SNS_SUCCESS;
}

AX_S32 os04d10_set_integration_time(ISP_PIPE_ID nPipeId, AX_SNS_AE_SHUTTER_CFG_T *ptIntTime)
{
    AX_U8 ex_h;
    AX_U8 ex_l;
    AX_U32 ex_ival = 0;
    AX_F32 fExpLineFromUser = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);
    SNS_CHECK_PTR_VALID(ptIntTime);

    SNS_DBG("Exptime:%.2f-%.2f-%.2f-%.2f, Hdrratio:%.2f-%.2f-%.2f-%.2f\n",
            ptIntTime->fIntTime[0], ptIntTime->fIntTime[1], ptIntTime->fIntTime[2], ptIntTime->fIntTime[3],
            ptIntTime->fHdrRatio[0], ptIntTime->fHdrRatio[1], ptIntTime->fHdrRatio[2], ptIntTime->fHdrRatio[3]);

    fExpLineFromUser = ptIntTime->fIntTime[HDR_LONG_FRAME_IDX];

    fExpLineFromUser = AXSNS_CLIP3(fExpLineFromUser,
                                    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMinIntegrationTime[HDR_LONG_FRAME_IDX],
                                    sns_obj->ae_ctrl_param.sns_ae_limit.tIntTimeRange.fMaxIntegrationTime[HDR_LONG_FRAME_IDX]);

    ex_ival = fExpLineFromUser;
    ex_l = REG_LOW_8BITS(ex_ival);
    ex_h = REG_HIGH_8BITS(ex_ival);
    os04d10_sns_update_regidx_table(nPipeId, OS04D10_PAGE_FLG_D2_IDX, OS04D10_PAGE_FLG_D2_P1);
    os04d10_sns_update_regidx_table(nPipeId, OS04D10_LONG_EXP_LINE_H_IDX, ex_h);
    os04d10_sns_update_regidx_table(nPipeId, OS04D10_LONG_EXP_LINE_L_IDX, ex_l);
    os04d10_sns_update_regidx_table(nPipeId, OS04D10_VBLANK_GAIN_EXP_EN_IDX, 0x01);

    sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX] = ex_ival;

    SNS_DBG("Exp line: long frame %f\n",
            sns_obj->ae_ctrl_param.sns_ae_param.fCurIntegrationTime[HDR_LONG_FRAME_IDX]);

    return SNS_SUCCESS;
}

AX_S32 os04d10_get_hw_exposure_params(ISP_PIPE_ID nPipeId, AX_SNS_EXP_CTRL_PARAM_T *ptAeCtrlParam)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(ptAeCtrlParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    memcpy(ptAeCtrlParam, &sns_obj->ae_ctrl_param, sizeof(AX_SNS_EXP_CTRL_PARAM_T));
    memcpy(&ptAeCtrlParam->sns_dev_attr, &sns_obj->sns_attr_param, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

/****************************************************************************
 * exposure control function
 ****************************************************************************/
AX_S32 os04d10_hcglcg_ctrl(ISP_PIPE_ID nPipeId, AX_U32 nSnsHcgLcg)
{
    AX_U8 hcglcg_value = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (AX_LCG_MODE == nSnsHcgLcg) {
        SNS_DBG("switch to LCG mode\n");
        sns_obj->ae_ctrl_param.eSnsHcgLcgMode = AX_LCG_MODE;
        hcglcg_value = 0x00;

        os04d10_sns_update_regidx_table(nPipeId, OS04D10_PAGE_FLG_D2_P1_IDX, OS04D10_PAGE_FLG_D2_P1);
        os04d10_sns_update_regidx_table(nPipeId, OS04D10_HCG_LCG_IDX, hcglcg_value);
    } else if (AX_HCG_MODE == nSnsHcgLcg) {
        SNS_DBG("switch to HCG mode \n");
        sns_obj->ae_ctrl_param.eSnsHcgLcgMode = AX_HCG_MODE;
        hcglcg_value = 0x02;

        os04d10_sns_update_regidx_table(nPipeId, OS04D10_PAGE_FLG_D2_P1_IDX, OS04D10_PAGE_FLG_D2_P1);
        os04d10_sns_update_regidx_table(nPipeId, OS04D10_HCG_LCG_IDX, hcglcg_value);
    }

    os04d10_sns_update_regidx_table(nPipeId, OS04D10_VBLANK_GAIN_EXP_EN_IDX, 0x01);

    return SNS_SUCCESS;
}

AX_S32 os04d10_sns_refresh_all_regs_from_tbl(ISP_PIPE_ID nPipeId)
{
    AX_S32 i = 0;
    AX_U32 nNum = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    nNum = sizeof(gOs04d10AeRegsTable) / sizeof(gOs04d10AeRegsTable[0]);

    for (i = 0; i < nNum; i++) {
        sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr = gOs04d10AeRegsTable[i].nRegAddr;
    }

    return SNS_SUCCESS;
}

AX_S32 os04d10_sns_update_init_exposure_reg(ISP_PIPE_ID nPipeId)
{
    int i = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        os04d10_write_register(nPipeId, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
        SNS_DBG("Idx = %d, reg addr 0x%x, reg data 0x%x\n",
            i, sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);
    }

    return SNS_SUCCESS;
}

static AX_VOID _sns_reg_info_init(ISP_PIPE_ID nPipeId, SNS_STATE_OBJ *sns_obj)
{
    AX_S32 i = 0;

    sns_obj->sztRegsInfo[0].eSnsType = ISP_SNS_CONNECT_I2C_TYPE;
    sns_obj->sztRegsInfo[0].tComBus.I2cDev = os04d10_get_bus_num(nPipeId);
    sns_obj->sztRegsInfo[0].nRegNum = sizeof(gOs04d10AeRegsTable) / sizeof(gOs04d10AeRegsTable[0]);
    sns_obj->sztRegsInfo[0].nCfg2ValidDelayMax = 2;

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDevAddr = gOs04D10SlaveAddr[nPipeId];
        sns_obj->sztRegsInfo[0].sztI2cData[i].nAddrByteNum = OS04D10_ADDR_BYTE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDataByteNum = OS04D10_DATA_BYTE;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nDelayFrmNum = gOs04d10AeRegsTable[i].nDelayFrmNum;
        sns_obj->sztRegsInfo[0].sztI2cData[i].nIntPos = AX_SNS_L_FSOF;
        SNS_DBG("[%2d] nRegAddr 0x%x, nRegValue 0x%x\n", i,
                gOs04d10AeRegsTable[i].nRegAddr, gOs04d10AeRegsTable[i].nRegValue);
    }

    return;
}

static AX_VOID _sns_reg_info_update(ISP_PIPE_ID nPipeId, SNS_STATE_OBJ *sns_obj, AX_BOOL *bUpdateReg)
{
    AX_S32 i = 0;
    sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_PAGE_FLG_D2_IDX].bUpdate = AX_FALSE;    //init 1 page
    sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_PAGE_FLG_D2_P5_IDX].bUpdate = AX_FALSE; //init 5 page
    sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_PAGE_FLG_D2_P1_IDX].bUpdate = AX_FALSE; //init 1 page

    for (i = 0; i < sns_obj->sztRegsInfo[0].nRegNum; i++) {
        if((i == OS04D10_PAGE_FLG_D2_IDX)     ||
           (i == OS04D10_PAGE_FLG_D2_P5_IDX)  ||
           (i == OS04D10_PAGE_FLG_D2_P1_IDX)) {
            continue;
            }

        if (sns_obj->sztRegsInfo[0].sztI2cData[i].nData == sns_obj->sztRegsInfo[1].sztI2cData[i].nData) {
                    sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_FALSE;
        } else {
            SNS_DBG("[%2d] nRegAddr 0x%x, nRegValue 0x%x\n", i,
                    sns_obj->sztRegsInfo[0].sztI2cData[i].nRegAddr, sns_obj->sztRegsInfo[0].sztI2cData[i].nData);

            sns_obj->sztRegsInfo[0].sztI2cData[i].bUpdate = AX_TRUE;
            *bUpdateReg = AX_TRUE;

            /* set again & shutter page*/
            if((i == OS04D10_LONG_AGAIN_IDX) ||
                (i == OS04D10_LONG_EXP_LINE_H_IDX) ||
                (i == OS04D10_LONG_EXP_LINE_L_IDX)) {
                sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_PAGE_FLG_D2_IDX].bUpdate = AX_TRUE; //set 1 page
            }

            /* set dgain page*/
            if((i == OS04D10_LONG_DGAIN_H_IDX) ||
                (i == OS04D10_LONG_DGAIN_L_IDX)) {
                sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_PAGE_FLG_D2_P5_IDX].bUpdate = AX_TRUE; //set 5 page
                sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_PAGE_FLG_D2_P1_IDX].bUpdate = AX_TRUE; //set 1 page
            }

            /* set vts page*/
            if((i == OS04D10_VBLANK_H_IDX) ||
                (i == OS04D10_VBLANK_L_IDX)) {
                sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_PAGE_FLG_D2_P1_IDX].bUpdate = AX_TRUE; //set 1 page
            }

            /* set hcglcg_switch page*/
            if(i == OS04D10_HCG_LCG_IDX) {
                sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_PAGE_FLG_D2_P1_IDX].bUpdate = AX_TRUE; //set 1 page
            }
        }
    }

    /* gain shutter vts hcglcg trigger */
    if (AX_TRUE == *bUpdateReg) {
        sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_VBLANK_GAIN_EXP_EN_IDX].bUpdate = AX_TRUE;  //trigger
    } else {
        sns_obj->sztRegsInfo[0].sztI2cData[OS04D10_VBLANK_GAIN_EXP_EN_IDX].bUpdate = AX_FALSE;
    }

    return;
}

AX_S32 os04d10_ae_get_sensor_reg_info(ISP_PIPE_ID nPipeId, AX_SNS_REGS_CFG_TABLE_T *ptSnsRegsInfo)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_BOOL bUpdateReg = AX_FALSE;

    SNS_CHECK_PTR_VALID(ptSnsRegsInfo);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if ((AX_FALSE == sns_obj->bSyncInit) || (AX_FALSE == ptSnsRegsInfo->bConfig)) {
        /* sync config */
        SNS_DBG(" bSyncInit %d, bConfig %d\n", sns_obj->bSyncInit, ptSnsRegsInfo->bConfig);

        _sns_reg_info_init(nPipeId, sns_obj);

        bUpdateReg = AX_TRUE;
        sns_obj->bSyncInit = AX_TRUE;
        // os04d10_sns_update_init_exposure_reg(nPipeId);
    } else {
        _sns_reg_info_update(nPipeId, sns_obj, &bUpdateReg);
    }

    if (AX_TRUE == bUpdateReg) {
        sns_obj->sztRegsInfo[0].bConfig = AX_FALSE;
    } else {
        sns_obj->sztRegsInfo[0].bConfig = AX_TRUE;
    }

    memcpy(ptSnsRegsInfo, &sns_obj->sztRegsInfo[0], sizeof(AX_SNS_REGS_CFG_TABLE_T));
    /* Save the current register table */
    memcpy(&sns_obj->sztRegsInfo[1], &sns_obj->sztRegsInfo[0], sizeof(AX_SNS_REGS_CFG_TABLE_T));

    return nRet;
}

AX_S32 os04d10_get_slow_shutter_param(ISP_PIPE_ID nPipeId, AX_SNS_AE_SLOW_SHUTTER_PARAM_T *ptSlowShutterParam)
{
    AX_S32 framerate = 30;
    AX_U32 nfps = 0;
    AX_U32 nVts = 0;

    SNS_STATE_OBJ *sns_obj = AX_NULL;


    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    framerate = sns_obj->sns_mode_obj.fFrameRate;
    if (SNS_MAX_FRAME_RATE < framerate) {
        SNS_ERR("framerate out of range : %d\n", framerate);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }
    if (sns_obj->ae_ctrl_param.fTimePerLine == 0) {
        SNS_ERR("line_period is zero : %f\n", sns_obj->ae_ctrl_param.fTimePerLine);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    ptSlowShutterParam->nGroupNum = AXSNS_MIN((sizeof(gFpsGear) / sizeof(AX_F32)), framerate);
    ptSlowShutterParam->fMinFps = AXSNS_MAX(gFpsGear[0],
                                            (1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * OS04D10_MAX_VTS)));

    for (nfps = 0 ; nfps < ptSlowShutterParam->nGroupNum; nfps++) {
        nVts = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * gFpsGear[nfps]);
        if ((AX_S32)gFpsGear[nfps] >= framerate) {
            nVts = sns_obj->sns_mode_obj.nVts;
        }
        if (nVts > OS04D10_MAX_VTS) {
            nVts = OS04D10_MAX_VTS;
            SNS_WRN("Beyond minmum fps  %f\n", ptSlowShutterParam->fMinFps);
        }

        ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime = OS04D10_SDR_EXP_LINE_MAX(nVts);
        ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine
                * nVts);
        ptSlowShutterParam->fMaxFps  =  ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps;

        SNS_DBG("nPipeId = %d, line_period = %.2f, fps = %.2f, fMaxIntTime = %.2f, vts=0x%x\n",
                nPipeId, sns_obj->ae_ctrl_param.fTimePerLine,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fRealFps,
                ptSlowShutterParam->tSlowShutterTbl[nfps].fMaxIntTime, nVts);
    }

    return SNS_SUCCESS;
}

AX_S32 os04d10_get_fps(ISP_PIPE_ID nPipeId, AX_F32 *pFps)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(pFps);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    *pFps = sns_obj->ae_ctrl_param.sns_ae_param.fCurFps;

    return SNS_SUCCESS;
}


AX_S32 os04d10_set_fps(ISP_PIPE_ID nPipeId, AX_F32 fFps)
{
    AX_U32 vts = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if(AXSNS_CAMPARE_FLOAT(AX_SNS_FPS_MIN, fFps) || AXSNS_CAMPARE_FLOAT(fFps, AX_SNS_FPS_MAX)) {
        SNS_ERR("pipe:%d, fps:%f is not supported, range:[%f-%f]\n", nPipeId, fFps, AX_SNS_FPS_MIN, AX_SNS_FPS_MAX);
        return -1;
    }

    if (IS_SNS_FPS_EQUAL(fFps, sns_os04d10params[nPipeId].setting_fps)) {
        vts = sns_os04d10params[nPipeId].setting_vts;
    } else {
        vts = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * fFps);
        if (vts > OS04D10_MAX_VTS){
            vts = OS04D10_MAX_VTS;
            SNS_WRN("Beyond max vts:0x%x\n", vts);
        }
    }

    /* vts can only be indirectly regulated through the vblank register
     * vblank_reg = vts - setting_vts
     */
    os04d10_set_vts(nPipeId, vts - sns_os04d10params[nPipeId].setting_vts);
    sns_os04d10params[nPipeId].vts = vts;
    if (IS_SNS_FPS_EQUAL(fFps, sns_obj->sns_attr_param.fFrameRate)) {
        sns_obj->sns_mode_obj.nVts = vts;
        sns_obj->sns_mode_obj.fFrameRate = sns_obj->sns_attr_param.fFrameRate;
    }

    os04d10_set_exp_limit(nPipeId, OS04D10_MAX_RATIO, sns_os04d10params[nPipeId].vts);
    sns_obj->ae_ctrl_param.sns_ae_param.fCurFps = 1 * SNS_1_SECOND_UNIT_US / (sns_obj->ae_ctrl_param.fTimePerLine * vts);

    SNS_DBG("pipe:%d, userFps:%f, curFps:%f, vts:0x%x\n",
        nPipeId, fFps, sns_obj->ae_ctrl_param.sns_ae_param.fCurFps, vts);

    return SNS_SUCCESS;
}
