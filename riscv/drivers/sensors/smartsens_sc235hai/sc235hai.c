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

#include "sc235hai_reg.h"
#include "sc235hai_ae_ctrl.h"

#include "sc235hai_sdr.h"
//#include "sc235hai_hdr_2x.h"

/****************************************************************************
 * golbal variables  and macro definition
 ****************************************************************************/

SNS_STATE_OBJ *g_szsc235haiCtx[AX_VIN_MAX_PIPE_NUM] = {NULL};

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szsc235haiCtx[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_szsc235haiCtx[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev) (g_szsc235haiCtx[dev] = NULL)


/****************************************************************************
 * Internal function definition
 ****************************************************************************/
static AX_S32 sc235hai_ctx_init(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 ret = 0;

    SNS_DBG("sc235hai sc235hai_ctx_init. ret = %d\n", ret);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        sns_obj = (SNS_STATE_OBJ *)calloc(1, sizeof(SNS_STATE_OBJ));
        if (AX_NULL == sns_obj) {
            SNS_ERR("malloc g_szsc235haiCtx failed\n");
            return AX_SNS_ERR_NOMEM;
        }
    }

    memset(sns_obj, 0, sizeof(SNS_STATE_OBJ));

    SENSOR_SET_CTX(nPipeId, sns_obj);

    return AX_SNS_SUCCESS;
}

static AX_VOID sc235hai_ctx_exit(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SENSOR_GET_CTX(nPipeId, sns_obj);
    free(sns_obj);
    SENSOR_RESET_CTX(nPipeId);
}

static AX_S32 sc235hai_get_chipid(ISP_PIPE_ID nPipeId, AX_S32 *pSnsId)
{
    AX_U32 sensor_id = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);

    if (sns_obj->sns_id == 0) {
        sensor_id |= sc235hai_reg_read(nPipeId, SC235HAI_SENSOR_ID_REG_H) << 8;
        sensor_id |= sc235hai_reg_read(nPipeId, SC235HAI_SENSOR_ID_REG_L);

        SNS_DBG("sc235hai id: 0x%x\n", sensor_id);

        if (sensor_id != SC235HAI_SENSOR_ID) {
            SNS_ERR("Failed to read sensor sc235hai id:0x%x\n", sensor_id);
            return SNS_ERR_CODE_ILLEGAL_PARAMS;
        }
        sns_obj->sns_id = sensor_id;
        *pSnsId = sensor_id;
    } else {
        *pSnsId = sns_obj->sns_id;
    }

    return AX_SNS_SUCCESS;
}

static void sc235hai_init(ISP_PIPE_ID nPipeId)
{
    AX_S32 nRet = 0;
    AX_S32 nSnsId = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    /* 1. contex init */
    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sc235hai_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sc235hai_ctx_init failed!\n");
            return;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    /* 2. i2c init */
    sc235hai_sensor_i2c_init(nPipeId);

    nRet = sc235hai_get_chipid(nPipeId, &nSnsId);
    if (nRet != AX_SNS_SUCCESS) {
        SNS_ERR("can't find sc235hai sensor id.\n");
    } else {
        SNS_DBG("sc235hai check chip id success.\n");
    }

    /* 3. config settings  */
    sc235hai_write_settings(nPipeId);

    /* 4. refresh ae param */
    sc235hai_cfg_aec_param(nPipeId);

    /* 5. refresh ae regs table */
    sc235hai_sns_refresh_all_regs_from_tbl(nPipeId);

    sns_obj->bSyncInit = AX_FALSE;
    sns_obj->sns_mode_obj.nVts = sc235hai_get_vts(nPipeId);
    return;
}

static void sc235hai_exit(ISP_PIPE_ID nPipeId)
{
    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    sc235hai_sensor_i2c_exit(nPipeId);
    sc235hai_ctx_exit(nPipeId);

    return;
}

AX_S32 sc235hai_sensor_streaming_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    if (1 == on) {
        result = sc235hai_write_register(nPipeId, 0x0100, 0x01); // stream on
        SNS_DBG("sensor stream on!\n");
    } else {
        result = sc235hai_write_register(nPipeId, 0x0100, 0x00); // stream off
        SNS_DBG("sensor stream off!\n");
    }
    if (result) {
        return result;
    }

    return AX_SNS_SUCCESS;
}

static AX_S32 sc235hai_sensor_set_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *sns_mode)
{
    AX_S32 ret = 0;
    AX_F32 setting_fps = 30.0f;
    AX_U32 sns_setting_index = 0;
    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_PTR_VALID(sns_mode);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        ret = sc235hai_ctx_init(nPipeId);
        if (0 != ret) {
            SNS_ERR("sc235hai_ctx_init failed!\n");
            return AX_SNS_ERR_NOT_INIT;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    if (sns_mode->nWidth == 1920 &&
        sns_mode->nHeight == 1080 &&
        sns_mode->eRawType == AX_RT_RAW10 &&
        sns_mode->eSnsMode == AX_SNS_LINEAR_MODE) {
        sns_setting_index = e_SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS;
        setting_fps = 60.0;
    } else if (sns_mode->nWidth == 1920 &&
        sns_mode->nHeight == 1080 &&
        sns_mode->eRawType == AX_RT_RAW10 &&
        sns_mode->eSnsMode == AX_SNS_HDR_2X_MODE) {
        sns_setting_index = e_SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS;
        setting_fps = 30.0;
    } else {
        SNS_ERR("it's not supported. nPipeId=%u, mode=%u_%u_%u_%f\n",
                nPipeId, sns_mode->eSnsMode, sns_mode->nWidth, sns_mode->nHeight, setting_fps);
        return AX_SNS_ERR_NOT_SUPPORT;
    }

    if (sns_mode->nSettingIndex > 0) {
        sns_setting_index = sns_mode->nSettingIndex;
        setting_fps = sns_mode->fFrameRate;
    }

    SNS_DBG("nPipeId=%u, sns_setting_index=%u, mode=%u_%u_%u_%f\n",
            nPipeId, sns_setting_index, sns_mode->eSnsMode, sns_mode->nWidth, sns_mode->nHeight, setting_fps);

    sns_obj->eImgMode = sns_setting_index;
    sns_obj->sns_mode_obj.eHDRMode = sns_mode->eSnsMode;
    sns_obj->sns_mode_obj.nWidth = sns_mode->nWidth;
    sns_obj->sns_mode_obj.nHeight = sns_mode->nHeight;
    sns_obj->sns_mode_obj.fFrameRate = setting_fps;
    memcpy(&sns_obj->sns_attr_param, sns_mode, sizeof(AX_SNS_ATTR_T));

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_sensor_get_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *pSnsMode)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(pSnsMode);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sc235hai_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sc235hai_ctx_init failed!\n");
            return AX_SNS_ERR_NOT_INIT;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    memcpy(pSnsMode, &sns_obj->sns_attr_param, sizeof(AX_SNS_ATTR_T));

    return AX_SNS_SUCCESS;
}

static AX_S32 sc235hai_testpattern_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SNS_DBG("test pattern enable: %d\n", on);
    if (1 == on) {
        /* enable test-pattern */
        sc235hai_write_register(nPipeId, 0x4501, 0xc5);
    } else {
        /* disable test-pattern */
        sc235hai_write_register(nPipeId, 0x4501, 0xc4);
    }

    return AX_SNS_SUCCESS;
}

static AX_S32 sc235hai_mirror_flip(ISP_PIPE_ID nPipeId, AX_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
    AX_S32 value = 0;

    value = sc235hai_reg_read(nPipeId, 0x3221);

    value &= 0x99;

    switch (eSnsMirrorFlip) {
    default:
    case AX_SNS_MF_NORMAL:
        sc235hai_write_register(nPipeId, 0x3221, 0x00 | value);
        break;

    case AX_SNS_MF_MIRROR:
        sc235hai_write_register(nPipeId, 0x3221, 0x06 | value);    //bit[2:1]
        break;

    case AX_SNS_MF_FLIP:
        sc235hai_write_register(nPipeId, 0x3221, 0x60 | value);    //bit[6:5]
        break;

    case AX_SNS_MF_MIRROR_FLIP:
        sc235hai_write_register(nPipeId, 0x3221, 0x66 | value);
        break;
    }
    return AX_SNS_SUCCESS;
}

/****************************************************************************
 * get module default parameters function
 ****************************************************************************/
static AX_S32 sc235hai_get_isp_default_params(ISP_PIPE_ID nPipeId, AX_SENSOR_DEFAULT_PARAM_T *ptDftParam)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_SNS_HDR_MODE_E nHdrmode;

    SNS_CHECK_PTR_VALID(ptDftParam);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);


    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        nRet = sc235hai_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sc235hai_ctx_init failed!\n");
            return AX_SNS_ERR_NOT_INIT;
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
        //ptDftParam->ptBlc           = (typeof(ptDftParam->ptBlc))&blc_param_hdr_2x;
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

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_get_black_level(ISP_PIPE_ID nPipeId, AX_SNS_BLACK_LEVEL_T *ptBlackLevel)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(ptBlackLevel);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    /* black level of linear mode */
    if (AX_SNS_LINEAR_MODE == sns_obj->sns_mode_obj.eHDRMode) {
        ptBlackLevel->nBlackLevel[0] = 1040;   /* linear mode 10bit sensor default blc 64(U10.0) --> 1024(U8.6) */
        ptBlackLevel->nBlackLevel[1] = 1040;
        ptBlackLevel->nBlackLevel[2] = 1040;
        ptBlackLevel->nBlackLevel[3] = 1040;
    } else {
        ptBlackLevel->nBlackLevel[0] = 1040;
        ptBlackLevel->nBlackLevel[1] = 1040;
        ptBlackLevel->nBlackLevel[2] = 1040;
        ptBlackLevel->nBlackLevel[3] = 1040;
    }

    return AX_SNS_SUCCESS;
}

AX_SYS_API_PUBLIC AX_SENSOR_REGISTER_FUNC_T gSnssc235haiObj = {

    /* sensor ctrl */
    .pfn_sensor_chipid                      = sc235hai_get_chipid,
    .pfn_sensor_init                        = sc235hai_init,
    .pfn_sensor_exit                        = sc235hai_exit,
    .pfn_sensor_reset                       = sc235hai_hw_reset,
    .pfn_sensor_streaming_ctrl              = sc235hai_sensor_streaming_ctrl,
    .pfn_sensor_testpattern                 = sc235hai_testpattern_ctrl,
    .pfn_sensor_mirror_flip                 = sc235hai_mirror_flip,

    .pfn_sensor_set_mode                    = sc235hai_sensor_set_mode,
    .pfn_sensor_get_mode                    = sc235hai_sensor_get_mode,
    .pfn_sensor_set_fps                     = sc235hai_set_fps,
    .pfn_sensor_get_fps                     = sc235hai_get_fps,
    .pfn_sensor_set_slaveaddr               = sc235hai_set_slaveaddr,

    /* communication : register read/write */
    .pfn_sensor_set_bus_info                = sc235hai_set_bus_info,
    .pfn_sensor_write_register              = sc235hai_write_register,
    .pfn_sensor_read_register               = sc235hai_read_register,

    /* default param */
    .pfn_sensor_get_default_params          = sc235hai_get_isp_default_params,
    .pfn_sensor_get_black_level             = sc235hai_get_black_level,

    /* ae ctrl */
    .pfn_sensor_get_hw_exposure_params      = sc235hai_get_hw_exposure_params,
    .pfn_sensor_get_gain_table              = sc235hai_get_sensor_gain_table,
    .pfn_sensor_set_again                   = sc235hai_set_again,
    .pfn_sensor_set_dgain                   = sc235hai_set_dgain,
    .pfn_sensor_hcglcg_ctrl                 = AX_NULL,

    .pfn_sensor_set_integration_time        = sc235hai_set_integration_time,
    .pfn_sensor_get_integration_time_range  = sc235hai_get_integration_time_range,
    .pfn_sensor_set_slow_fps                = sc235hai_set_fps,
    .pfn_sensor_get_slow_shutter_param      = sc235hai_get_slow_shutter_param,
    .pfn_sensor_get_sns_reg_info            = sc235hai_ae_get_sensor_reg_info,
};
