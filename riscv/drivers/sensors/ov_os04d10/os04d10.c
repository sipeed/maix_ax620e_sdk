/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_base_type.h"
#include "ax_isp_common.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


#include "ax_sensor_struct.h"
#include "drv_i2c.h"

#include "isp_sensor_internal.h"
#include "isp_sensor_types.h"

#include "os04d10_reg.h"
#include "os04d10_ae_ctrl.h"

/* default param */
#include "os04d10_sdr.h"

/****************************************************************************
 * golbal variables  and macro definition                                   *
 ****************************************************************************/

SNS_STATE_OBJ *g_szOs04d10Ctx[AX_VIN_MAX_PIPE_NUM] = {AX_NULL};

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs04d10Ctx[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_szOs04d10Ctx[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev) (g_szOs04d10Ctx[dev] = AX_NULL)

/****************************************************************************
 * Internal function definition
 ****************************************************************************/
static AX_S32 sensor_ctx_init(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 ret = 0;

    SNS_DBG("os04d10 sensor_ctx_init. ret = %d\n", ret);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);

    if (AX_NULL == sns_obj) {
        sns_obj = (SNS_STATE_OBJ *)calloc(1, sizeof(SNS_STATE_OBJ));
        if (AX_NULL == sns_obj) {
            SNS_ERR("malloc g_szOs04d10Ctx failed\r\n");
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
AX_S32 os04d10_get_chipid(ISP_PIPE_ID nPipeId, AX_S32 *pSnsId)
{
    AX_U64 sensor_id = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    os04d10_write_register(nPipeId, OS04D10_PAGE_FLG_D2, OS04D10_PAGE_FLG_D2_P0);

    sensor_id |= os04d10_reg_read(nPipeId, 0x02) << 24;
    sensor_id |= os04d10_reg_read(nPipeId, 0x03) << 16;
    sensor_id |= os04d10_reg_read(nPipeId, 0x04) << 8;
    sensor_id |= os04d10_reg_read(nPipeId, 0x05);

    SNS_DBG("%s: sensor os04d10 id: 0x%x\n", __func__, sensor_id);

    if (sensor_id != OS04D10_SENSOR_CHIP_ID) {
        SNS_ERR("%s: Failed to read sensor os04d10 id\n", __func__);
        return -1;
    }

    *pSnsId = sensor_id;

    return SNS_SUCCESS;
}

static void os04d10_init(ISP_PIPE_ID nPipeId)
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
        nRet = sensor_ctx_init(nPipeId);
        if (0 != nRet) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    /* 2. i2c init */
    os04d10_sensor_i2c_init(nPipeId);

    nRet = os04d10_get_chipid(nPipeId, &nSnsId);
    if (nRet != SNS_SUCCESS) {
        SNS_ERR("can't find os04d10 sensor id.\r\n");
    } else {
        SNS_DBG("os04d10 check chip id success.\r\n");
    }

    /* 3. config settings  */
    os04d10_write_settings(nPipeId);

    /* 4. refresh ae param */
    os04d10_cfg_aec_param(nPipeId);

    /* 5. refresh ae regs table */
    os04d10_sns_refresh_all_regs_from_tbl(nPipeId);

    sns_obj->bSyncInit = AX_FALSE;

    return;
}

static void os04d10_exit(ISP_PIPE_ID nPipeId)
{
    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    os04d10_sensor_i2c_exit(nPipeId);
    sensor_ctx_exit(nPipeId);

    return;
}


AX_S32 os04d10_sensor_streaming_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    AX_S32 result = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    result |= os04d10_write_register(nPipeId, OS04D10_PAGE_FLG_D2, OS04D10_PAGE_FLG_D2_P5);

    if (1 == on) {
        result |= os04d10_write_register(nPipeId, OS04D10_MIPI_EN, 0x01); // stream on
        SNS_DBG("sensor stream on!\n");
    } else {
        result |= os04d10_write_register(nPipeId, OS04D10_MIPI_EN, 0x00); // stream off
        SNS_DBG("sensor stream off!\n");
    }
    if (result) {
        return result;
    }

    return SNS_SUCCESS;
}

AX_S32 os04d10_sensor_set_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *sns_mode)
{
    AX_S32 nRet = 0;
    AX_S32 sns_setting_index = 0;
    AX_F32 setting_fps = 30.0f;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(sns_mode);
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

    if (sns_mode->nWidth == 2560            &&
        sns_mode->nHeight == 1440           &&
        sns_mode->eRawType == AX_RT_RAW10   &&
        sns_mode->eSnsMode == AX_SNS_LINEAR_MODE)
    {
        sns_setting_index = e_OS04D10_2lane_2560x1440_10bit_Linear_30fps;
        setting_fps = 30;
    } else if (sns_mode->nWidth == 1280        &&
               sns_mode->nHeight == 720        &&
               sns_mode->eRawType == AX_RT_RAW10   &&
               sns_mode->eSnsMode == AX_SNS_LINEAR_MODE){
                sns_setting_index = e_OS04D10_2lane_1280x720_10bit_Linear_60fps;
                setting_fps = 60;
    } else {
        SNS_ERR("it's not supported. pipe=%u, mode=%u_%u_%u_%f\n",
            nPipeId, sns_mode->eSnsMode, sns_mode->nWidth, sns_mode->nHeight, sns_mode->fFrameRate);
        return -1;
    }

    /* optional, Not Recommended. if nSettingIndex > 0 will take effect */
    if (sns_mode->nSettingIndex > 0) {
        sns_setting_index = sns_mode->nSettingIndex;
        setting_fps = sns_mode->fFrameRate;
    }

    SNS_DBG("pipe=%u, sns_setting_index=%u\n", nPipeId, sns_setting_index);
    sns_obj->eImgMode = sns_setting_index;
    sns_obj->sns_mode_obj.eHDRMode = sns_mode->eSnsMode;
    sns_obj->sns_mode_obj.nWidth = sns_mode->nWidth;
    sns_obj->sns_mode_obj.nHeight = sns_mode->nHeight;
    sns_obj->sns_mode_obj.fFrameRate = setting_fps;
    memcpy(&sns_obj->sns_attr_param, sns_mode, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

AX_S32 os04d10_sensor_get_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *pSnsMode)
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


AX_S32 os04d10_testpattern_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SNS_DBG("test pattern enable: %d\n", on);
    os04d10_write_register(nPipeId, OS04D10_PAGE_FLG_D2, OS04D10_PAGE_FLG_D2_P5);
    if (1 == on) {
        /* enable test-pattern */
        os04d10_write_register(nPipeId, OS04D10_TEST_PARTTEN_EN, 0x01);
    } else {
        /* disable test-pattern */
        os04d10_write_register(nPipeId, OS04D10_TEST_PARTTEN_EN, 0x00);
    }

    return SNS_SUCCESS;
}


/****************************************************************************
 * get module default parameters function
 ****************************************************************************/
AX_S32 os04d10_get_isp_default_params(ISP_PIPE_ID nPipeId, AX_SENSOR_DEFAULT_PARAM_T *ptDftParam)
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
            return -1;
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

AX_S32 os04d10_get_black_level(ISP_PIPE_ID nPipeId, AX_SNS_BLACK_LEVEL_T *ptBlackLevel)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_PTR_VALID(ptBlackLevel);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    /* P5:0xF0-0xF3,  0x40 */
    /* 8bit 16->10bit 64->12bit 256->14bit 1024(U8.6) */
    if (AX_SNS_LINEAR_MODE == sns_obj->sns_mode_obj.eHDRMode) {
        ptBlackLevel->nBlackLevel[0] = 1024;
        ptBlackLevel->nBlackLevel[1] = 1024;
        ptBlackLevel->nBlackLevel[2] = 1024;
        ptBlackLevel->nBlackLevel[3] = 1024;
    } else {
        ptBlackLevel->nBlackLevel[0] = 1024;
        ptBlackLevel->nBlackLevel[1] = 1024;
        ptBlackLevel->nBlackLevel[2] = 1024;
        ptBlackLevel->nBlackLevel[3] = 1024;
    }

    return SNS_SUCCESS;
}

AX_SYS_API_PUBLIC AX_SENSOR_REGISTER_FUNC_T gSnsos04d10Obj = {

    /* sensor ctrl */
    .pfn_sensor_reset                       = os04d10_reset,
    .pfn_sensor_chipid                      = os04d10_get_chipid,
    .pfn_sensor_init                        = os04d10_init,
    .pfn_sensor_exit                        = os04d10_exit,
    .pfn_sensor_streaming_ctrl              = os04d10_sensor_streaming_ctrl,
    .pfn_sensor_testpattern                 = os04d10_testpattern_ctrl,

    .pfn_sensor_set_mode                    = os04d10_sensor_set_mode,
    .pfn_sensor_get_mode                    = os04d10_sensor_get_mode,

    .pfn_sensor_set_fps                     = os04d10_set_fps,
    .pfn_sensor_get_fps                     = os04d10_get_fps,
    .pfn_sensor_set_slaveaddr               = os04d10_set_slaveaddr,

    /* communication : register read/write */
    .pfn_sensor_set_bus_info                = os04d10_set_bus_info,
    .pfn_sensor_write_register              = os04d10_write_register,
    .pfn_sensor_read_register               = os04d10_read_register,

    /* default param */
    .pfn_sensor_get_default_params          = os04d10_get_isp_default_params,
    .pfn_sensor_get_black_level             = os04d10_get_black_level,

    /* ae ctrl */
    .pfn_sensor_get_hw_exposure_params      = os04d10_get_hw_exposure_params,
    .pfn_sensor_get_gain_table              = os04d10_get_sensor_gain_table,
    .pfn_sensor_set_again                   = os04d10_set_again,
    .pfn_sensor_set_dgain                   = os04d10_set_dgain,
    .pfn_sensor_hcglcg_ctrl                 = os04d10_hcglcg_ctrl,

    .pfn_sensor_set_integration_time        = os04d10_set_integration_time,
    .pfn_sensor_get_integration_time_range  = os04d10_get_integration_time_range,
    .pfn_sensor_set_slow_fps                = os04d10_set_fps,
    .pfn_sensor_get_slow_shutter_param      = os04d10_get_slow_shutter_param,
    .pfn_sensor_get_sns_reg_info            = os04d10_ae_get_sensor_reg_info,
    .pfn_sensor_set_wbgain                  = AX_NULL,
};
