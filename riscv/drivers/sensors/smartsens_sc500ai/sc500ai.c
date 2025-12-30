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
#include <stdlib.h>

#include "ax_sensor_struct.h"
#include "ax_base_type.h"
#include "ax_isp_common.h"
#include "isp_sensor_internal.h"
#include "isp_sensor_types.h"
#include "isp_sensor_internal.h"

#include "sc500ai_reg.h"
#include "sc500ai_ae_ctrl.h"

/* default param */
#include "sc500ai_sdr.h"
#include "sc500ai_hdr_2x.h"

/****************************************************************************
 * golbal variables  and macro definition                                   *
 ****************************************************************************/

SNS_STATE_OBJ *gSc500aiCtx[AX_VIN_MAX_PIPE_NUM];

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = gSc500aiCtx[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (gSc500aiCtx[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev) (gSc500aiCtx[dev] = AX_NULL)

/****************************************************************************
 * Internal function definition
 ****************************************************************************/
static AX_S32 sensor_ctx_init(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 ret = 0;

    SNS_DBG("sc500ai sensor_ctx_init. ret = %d\n", ret);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);

    if (AX_NULL == sns_obj) {
        sns_obj = (SNS_STATE_OBJ *)calloc(1, sizeof(SNS_STATE_OBJ));
        if (AX_NULL == sns_obj) {
            SNS_ERR("malloc gSc500aiCtx failed\r\n");
            return SNS_ERR_CODE_NOT_MEM;
        }
    }

    memset(sns_obj, 0, sizeof(SNS_STATE_OBJ));

    SENSOR_SET_CTX(nPipeId, sns_obj);

    return SNS_SUCCESS;
}

static AX_VOID sensor_ctx_exit(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SENSOR_GET_CTX(nPipeId, sns_obj);
    free(sns_obj);
    SENSOR_RESET_CTX(nPipeId);
}

/****************************************************************************
 * sensor control function
 ****************************************************************************/
static AX_S32 sc500ai_get_chipid(ISP_PIPE_ID nPipeId, AX_S32 *pSnsId)
{
    AX_U16 sensor_id = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    sensor_id |= sc500ai_reg_read(nPipeId, 0x3107) << 8;
    sensor_id |= sc500ai_reg_read(nPipeId, 0x3108);

    SNS_DBG("sc500ai id: 0x%x\n", sensor_id);

    if (sensor_id != SC500AI_SENSOR_ID) {
        SNS_ERR("Failed to read sensor sc500ai id:0x%x\n", sensor_id);
        return -1;
    }
    return 0;
}

static void sc500ai_init(ISP_PIPE_ID nPipeId)
{
    AX_S32 nRet = 0;
    AX_S32 nImagemode = 0;
    AX_S32 nSnsId = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    /* 1. contex init */
    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    /* 2. i2c init */
    sc500ai_i2c_init(nPipeId);

    sc500ai_get_chipid(nPipeId, &nSnsId);

    /* 3. config settings  */
    nImagemode = sns_obj->eImgMode;
    sc500ai_write_settings(nPipeId, nImagemode);

    /* 4. refresh ae param */
    sc500ai_cfg_aec_param(nPipeId);

    /* 5. refresh ae regs table */
    sc500ai_refresh_all_regs_from_tbl(nPipeId);

    sns_obj->bSyncInit = AX_FALSE;

    return;
}

static void sc500ai_exit(ISP_PIPE_ID nPipeId)
{
    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    sc500ai_i2c_exit(nPipeId);
    sensor_ctx_exit(nPipeId);

    return;
}


static AX_S32 sc500ai_stream_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    AX_S32 ret = 0;
    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    SNS_DBG("pipe:%d, sensor stream on:%d\n", nPipeId, on);

    if (1 == on) {
        ret = sc500ai_write_register(nPipeId, 0x0100, 0x01); // stream on
    } else {
        ret = sc500ai_write_register(nPipeId, 0x0100, 0x00); // stream off
    }
    if (0 != ret) {
        return -1;
    }

    return SNS_SUCCESS;
}

static AX_S32 sc500ai_set_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *sns_mode)
{
    AX_S32 nRet = 0;
    AX_S32 sns_setting_index = 0;
    AX_F32 setting_fps = 30;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(sns_mode);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return SNS_ERR_CODE_INIT_FAILD;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    sns_obj->bSyncInit = AX_FALSE;

    if (sns_mode->nWidth == 1440            &&
        sns_mode->nHeight == 810            &&
        sns_mode->eRawType == AX_RT_RAW10   &&
        sns_mode->eSnsMode == AX_SNS_LINEAR_MODE)
    {
        sns_setting_index = E_SC500AI_MIPI_4LANE_24M_450MBPS_1440x810_10BIT_SDR_60FPS;
        setting_fps = 60;

    } else if (sns_mode->nWidth == 2880     &&
        sns_mode->nHeight == 1620           &&
        sns_mode->eRawType == AX_RT_RAW10   &&
        sns_mode->eSnsMode == AX_SNS_LINEAR_MODE)
    {
        sns_setting_index = E_SC500AI_MIPI_4LANE_24M_396MBPS_2880x1620_10BIT_SDR_30FPS;
        setting_fps = 30;

    } else {
        SNS_ERR("it's not supported. pipe=%d, mode=%d_%d_%d_%d\n",
            nPipeId, sns_mode->eSnsMode, sns_mode->nWidth, sns_mode->nHeight, sns_mode->fFrameRate);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    /* optional, Not Recommended. if nSettingIndex > 0 will take effect */
    if (sns_mode->nSettingIndex > 0) {
        sns_setting_index = sns_mode->nSettingIndex;
        setting_fps = sns_mode->fFrameRate;
    }

    SNS_DBG("pipe=%d, sns_setting_index=%d\n", nPipeId, sns_setting_index);
    sns_obj->eImgMode = sns_setting_index;
    sns_obj->sns_mode_obj.eHDRMode = sns_mode->eSnsMode;
    sns_obj->sns_mode_obj.nWidth = sns_mode->nWidth;
    sns_obj->sns_mode_obj.nHeight = sns_mode->nHeight;
    sns_obj->sns_mode_obj.fFrameRate = setting_fps;
    memcpy(&sns_obj->sns_attr_param, sns_mode, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

static AX_S32 sc500ai_get_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *pSnsMode)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(pSnsMode);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return -1;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    memcpy(pSnsMode, &sns_obj->sns_attr_param, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}


static AX_S32 sc500ai_testpattern_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    AX_S32 value = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SNS_DBG("nPipeId:%d, testpattern on:%d\n", nPipeId, on);

    value = sc500ai_reg_read(nPipeId, 0x4501);

    value &= 0xF7;

    if (1 == on) {
        sc500ai_write_register(nPipeId, 0x4501, 0x08 | value); //bit3=1
    } else {
        sc500ai_write_register(nPipeId, 0x4501, 0x00 | value);
    }

    return SNS_SUCCESS;
}

static AX_S32 sc500ai_mirror_flip(ISP_PIPE_ID nPipeId, AX_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
    AX_S32 value = 0;

    value = sc500ai_reg_read(nPipeId, 0x3221);

    value &= 0x99;

    switch (eSnsMirrorFlip) {
    default:
    case AX_SNS_MF_NORMAL:
        sc500ai_write_register(nPipeId, 0x3221, 0x00 | value);
        break;

    case AX_SNS_MF_MIRROR:
        sc500ai_write_register(nPipeId, 0x3221, 0x06 | value);    //bit[2:1]
        break;

    case AX_SNS_MF_FLIP:
        sc500ai_write_register(nPipeId, 0x3221, 0x60 | value);    //bit[6:5]
        break;

    case AX_SNS_MF_MIRROR_FLIP:
        sc500ai_write_register(nPipeId, 0x3221, 0x66 | value);
        break;
    }
    return SNS_SUCCESS;
}

/****************************************************************************
 * get module default parameters function
 ****************************************************************************/
static AX_S32 sc500ai_get_isp_default_params(ISP_PIPE_ID nPipeId, AX_SENSOR_DEFAULT_PARAM_T *ptDftParam)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_SNS_HDR_MODE_E nHdrmode;

    SNS_CHECK_PTR_VALID(ptDftParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);


    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sensor_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return SNS_ERR_CODE_INIT_FAILD;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    memset(ptDftParam, 0, sizeof(AX_SENSOR_DEFAULT_PARAM_T));

    nHdrmode = sns_obj->sns_mode_obj.eHDRMode;

    SNS_DBG(" current hdr mode %d \n", nHdrmode);

    switch (nHdrmode) {
    case AX_SNS_LINEAR_MODE:
        /* TODO: Users configure their own default parameters */
        ptDftParam->ptAeDftParam    = (typeof(ptDftParam->ptAeDftParam))&ae_param_sdr;
        ptDftParam->ptAwbDftParam   = (typeof(ptDftParam->ptAwbDftParam))&awb_param_sdr;
        ptDftParam->ptBlc           = (typeof(ptDftParam->ptBlc))&blc_param_sdr;
        break;

    case AX_SNS_HDR_2X_MODE:
        /* TODO: Users configure their own default parameters */
        ptDftParam->ptAeDftParam    = (typeof(ptDftParam->ptAeDftParam))&ae_param_hdr_2x;
        break;

    case AX_SNS_HDR_3X_MODE:
        /* TODO: Users configure their own default parameters */
        break;

    case AX_SNS_HDR_4X_MODE:
        /* TODO: Users configure their own default parameters */
        break;
    default:
        SNS_ERR(" hdr mode %d error\n", nHdrmode);
        break;
    }

    return SNS_SUCCESS;
}

static AX_S32 sc500ai_get_black_level(ISP_PIPE_ID nPipeId, AX_SNS_BLACK_LEVEL_T *ptBlackLevel)
{
    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_PTR_VALID(ptBlackLevel);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    //{0x3907/0x3908}=0x41
    if (AX_SNS_LINEAR_MODE == sns_obj->sns_mode_obj.eHDRMode) {
        ptBlackLevel->nBlackLevel[0] = 1040;    /* 10bit 0x41(U10.0) --> 1040(U8.6) */
        ptBlackLevel->nBlackLevel[1] = 1040;
        ptBlackLevel->nBlackLevel[2] = 1040;
        ptBlackLevel->nBlackLevel[3] = 1040;
    } else {
        ptBlackLevel->nBlackLevel[0] = 1040;
        ptBlackLevel->nBlackLevel[1] = 1040;
        ptBlackLevel->nBlackLevel[2] = 1040;
        ptBlackLevel->nBlackLevel[3] = 1040;
    }

    return SNS_SUCCESS;
}


AX_SYS_API_PUBLIC AX_SENSOR_REGISTER_FUNC_T gSnssc500aiObj = {

    /* sensor ctrl */
    .pfn_sensor_reset                       = sc500ai_reset,
    .pfn_sensor_chipid                      = sc500ai_get_chipid,
    .pfn_sensor_init                        = sc500ai_init,
    .pfn_sensor_exit                        = sc500ai_exit,
    .pfn_sensor_streaming_ctrl              = sc500ai_stream_ctrl,
    .pfn_sensor_testpattern                 = sc500ai_testpattern_ctrl,
    .pfn_sensor_mirror_flip                 = sc500ai_mirror_flip,

    .pfn_sensor_set_mode                    = sc500ai_set_mode,
    .pfn_sensor_get_mode                    = sc500ai_get_mode,

    .pfn_sensor_set_fps                     = sc500ai_set_fps,
    .pfn_sensor_set_slaveaddr               = sc500ai_set_slaveaddr,

    /* communication : register read/write */
    .pfn_sensor_set_bus_info                = sc500ai_set_bus_info,
    .pfn_sensor_write_register              = sc500ai_write_register,
    .pfn_sensor_read_register               = sc500ai_read_register,

    /* default param */
    .pfn_sensor_get_default_params          = sc500ai_get_isp_default_params,
    .pfn_sensor_get_black_level             = sc500ai_get_black_level,

    /* ae ctrl */
    .pfn_sensor_get_hw_exposure_params      = sc500ai_get_hw_exposure_params,
    .pfn_sensor_get_gain_table              = sc500ai_get_gain_table,
    .pfn_sensor_set_again                   = sc500ai_set_again,
    .pfn_sensor_set_dgain                   = sc500ai_set_dgain,
    .pfn_sensor_hcglcg_ctrl                 = sc500ai_hcglcg_ctrl,

    .pfn_sensor_set_integration_time        = sc500ai_set_integration_time,
    .pfn_sensor_get_integration_time_range  = sc500ai_get_integration_time_range,
    .pfn_sensor_set_slow_fps                = sc500ai_set_slow_fps,
    .pfn_sensor_get_slow_shutter_param      = sc500ai_get_slow_shutter_param,
    .pfn_sensor_get_sns_reg_info            = sc500ai_ae_get_sensor_reg_info,
};
