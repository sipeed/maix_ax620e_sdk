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

#include "sc200ai_reg.h"
#include "sc200ai_ae_ctrl.h"

/* default param */
#include "sc200ai_sdr.h"
#include "sc200ai_hdr_2x.h"

// #include "ax_module_version.h"
/****************************************************************************
 * golbal variables  and macro definition
 ****************************************************************************/

SNS_STATE_OBJ *g_szsc200aiCtx[AX_VIN_MAX_PIPE_NUM] = {NULL};

#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szsc200aiCtx[dev])
#define SENSOR_SET_CTX(dev, pstCtx) (g_szsc200aiCtx[dev] = pstCtx)
#define SENSOR_RESET_CTX(dev) (g_szsc200aiCtx[dev] = NULL)

/****************************************************************************
 * Internal function definition
 ****************************************************************************/
static AX_S32 sensor_ctx_init(ISP_PIPE_ID nPipeId)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 ret = 0;

    SNS_DBG("sc200ai sensor_ctx_init. ret = %d\n", ret);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        sns_obj = (SNS_STATE_OBJ *)calloc(1, sizeof(SNS_STATE_OBJ));
        if (AX_NULL == sns_obj) {
            SNS_ERR("malloc g_szsc200aiCtx failed\r\n");
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

#if 0
/****************************************************************************
 * sensor control function
 ****************************************************************************/
static AX_S32 sc200ai_get_version(ISP_PIPE_ID nPipeId, AX_CHAR * ptSnsVerInfo)
{
    SNS_CHECK_PTR_VALID(ptSnsVerInfo);

    memcpy(ptSnsVerInfo, axera_module_version, sizeof(axera_module_version));

    return 0;
}
#endif

static AX_S32 sc200ai_get_chipid(ISP_PIPE_ID nPipeId, AX_S32 *pSnsId)
{
    AX_U32 sensor_id = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    sensor_id |= sc200ai_reg_read(nPipeId, 0x3107) << 8;
    sensor_id |= sc200ai_reg_read(nPipeId, 0x3108);

    SNS_DBG("%s: sensor sc200ai id: 0x%x\n", __func__, sensor_id);

    if (sensor_id != SC200AI_SENSOR_CHIP_ID) {
        SNS_ERR("%s: Failed to read sensor sc200ai id 0x%x\n", __func__, sensor_id);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    *pSnsId = sensor_id;

    return SNS_SUCCESS;
}

static void sc200ai_init(ISP_PIPE_ID nPipeId)
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
    sc200ai_sensor_i2c_init(nPipeId);

    nRet = sc200ai_get_chipid(nPipeId, &nSnsId);
    if (nRet != SNS_SUCCESS) {
        SNS_ERR("can't find sc200ai sensor id.\r\n");
    } else {
        SNS_DBG("sc200ai check chip id success.\r\n");
    }

    /* 3. config settings  */
    nImagemode = sns_obj->eImgMode;
    sc200ai_write_settings(nPipeId, nImagemode);

    /* 4. refresh ae param */
    sc200ai_cfg_aec_param(nPipeId);

    /* 5. refresh ae regs table */
    sc200ai_sns_refresh_all_regs_from_tbl(nPipeId);
    sns_obj->bSyncInit = AX_FALSE;
    sns_obj->sns_mode_obj.nVts = sc200ai_get_vts(nPipeId);

    return;
}

static void sc200ai_exit(ISP_PIPE_ID nPipeId)
{
    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM)) {
        return;
    }

    sc200ai_sensor_i2c_exit(nPipeId);
    sensor_ctx_exit(nPipeId);

    return;
}

static AX_S32 sc200ai_sensor_streaming_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    if (1 == on) {
        result = sc200ai_write_register(nPipeId, 0x0100, 0x01); // stream on
        SNS_DBG("sensor stream on!\n");
    } else {
        result = sc200ai_write_register(nPipeId, 0x0100, 0x00); // stream off
        SNS_DBG("sensor stream off!\n");
    }
    if (0 != result) {
        return -1;
    }

    return SNS_SUCCESS;
}

static AX_S32 sc200ai_sensor_set_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *sns_mode)
{
    AX_S32 ret = 0;
    AX_U32 width;
    AX_U32 height;
    AX_U32 hdrmode;
    AX_S32 fps = 25;
    AX_U32 sns_seting_index = 0;
    SNS_STATE_OBJ *sns_obj = NULL;

    SNS_CHECK_PTR_VALID(sns_mode);
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    if (AX_NULL == sns_obj) {
        /* contex init */
        ret = sensor_ctx_init(nPipeId);
        if (0 != ret) {
            SNS_ERR("sensor_ctx_init failed!\n");
            return SNS_ERR_CODE_INIT_FAILD;
        } else {
            SENSOR_GET_CTX(nPipeId, sns_obj);
        }
    }

    sns_obj->bSyncInit = AX_FALSE;
    width = sns_mode->nWidth;
    height = sns_mode->nHeight;
    fps = sns_mode->fFrameRate;
    hdrmode = sns_mode->eSnsMode;

    if (AX_SNS_SYNC_MASTER == sns_mode->eMasterSlaveSel) {
        if (width == 1920 && height == 1080 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 25.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_MASTER;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 30.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_MASTER;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_HDR_2X_MODE && IS_SNS_FPS_EQUAL(fps, 25.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_MASTER;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_HDR_2X_MODE && IS_SNS_FPS_EQUAL(fps, 30.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_MASTER;
        } else {
            SNS_ERR("not support sync slave, pipe:%d, mode=%d_%d_%d_%f\n", nPipeId, hdrmode, width, height, fps);
            return SNS_ERR_CODE_ILLEGAL_PARAMS;
        }
    } else if (AX_SNS_SYNC_SLAVE == sns_mode->eMasterSlaveSel) {
        if (width == 1920 && height == 1080 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 25.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_SLAVE;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 30.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_SLAVE;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_HDR_2X_MODE && IS_SNS_FPS_EQUAL(fps, 25.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_SLAVE;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_HDR_2X_MODE && IS_SNS_FPS_EQUAL(fps, 30.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_SLAVE;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 60.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS_SYNC_SLAVE;
        } else {
            SNS_ERR("not support sync slave, pipe:%d, mode=%d_%d_%d_%f\n", nPipeId, hdrmode, width, height, fps);
            return SNS_ERR_CODE_ILLEGAL_PARAMS;
        }
    } else {
        if (width == 1920 && height == 1080 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 60.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 25.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 30.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_HDR_2X_MODE && IS_SNS_FPS_EQUAL(fps, 25.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS;
        } else if (width == 1920 && height == 1080 && hdrmode == AX_SNS_HDR_2X_MODE && IS_SNS_FPS_EQUAL(fps, 30.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS;
        } else if (width == 1920 && height == 1088 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 25.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_25FPS;
        } else if (width == 1920 && height == 1088 && hdrmode == AX_SNS_LINEAR_MODE && IS_SNS_FPS_EQUAL(fps, 30.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_30FPS;
        } else if (width == 1920 && height == 1088 && hdrmode == AX_SNS_HDR_2X_MODE && IS_SNS_FPS_EQUAL(fps, 25.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_25FPS;
        } else if (width == 1920 && height == 1088 && hdrmode == AX_SNS_HDR_2X_MODE && IS_SNS_FPS_EQUAL(fps, 30.0)) {
            sns_seting_index = e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_30FPS;
        } else {
            SNS_ERR("it's not supported. nPipeId=%d, mode=%d_%d_%d_%f\n", nPipeId, hdrmode, width, height, fps);
            return SNS_ERR_CODE_ILLEGAL_PARAMS;
        }
    }

    if (sns_mode->nSettingIndex > 0) {
        sns_seting_index = sns_mode->nSettingIndex;
    }

    SNS_DBG("nPipeId=%d, sns_seting_index=%d, mode=%d_%d_%d_%d\n", nPipeId, sns_seting_index, hdrmode, width, height, fps);
    sns_obj->eImgMode = sns_seting_index;
    sns_obj->sns_mode_obj.eHDRMode = hdrmode;
    sns_obj->sns_mode_obj.nWidth = width;
    sns_obj->sns_mode_obj.nHeight = height;
    sns_obj->sns_mode_obj.fFrameRate = fps;
    sns_obj->sns_mode_obj.eMasterSlaveSel = sns_mode->eMasterSlaveSel;
    memcpy(&sns_obj->sns_attr_param, sns_mode, sizeof(AX_SNS_ATTR_T));

    return SNS_SUCCESS;
}

static AX_S32 sc200ai_sensor_get_mode(ISP_PIPE_ID nPipeId, AX_SNS_ATTR_T *pSnsMode)
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

static AX_S32 sc200ai_testpattern_ctrl(ISP_PIPE_ID nPipeId, AX_U32 on)
{
    AX_S32 value = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SNS_DBG("test pattern enable: %d\n", on);

    value = sc200ai_reg_read(nPipeId, 0x4501);

    value &= 0xF7;

    if (1 == on) {
        sc200ai_write_register(nPipeId, 0x4501, 0x08 | value); //bit3=1
    } else {
        sc200ai_write_register(nPipeId, 0x4501, 0x00 | value);
    }

    return SNS_SUCCESS;
}

/****************************************************************************
 * get module default parameters function
 ****************************************************************************/
static AX_S32 sc200ai_get_isp_default_params(ISP_PIPE_ID nPipeId, AX_SENSOR_DEFAULT_PARAM_T *ptDftParam)
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

static AX_S32 sc200ai_get_black_level(ISP_PIPE_ID nPipeId, AX_SNS_BLACK_LEVEL_T *ptBlackLevel)
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

    return SNS_SUCCESS;
}


AX_SYS_API_PUBLIC AX_SENSOR_REGISTER_FUNC_T gSnssc200aiObj = {

    /* sensor ctrl */
    .pfn_sensor_chipid                      = sc200ai_get_chipid,
    .pfn_sensor_init                        = sc200ai_init,
    .pfn_sensor_exit                        = sc200ai_exit,
    .pfn_sensor_reset                       = sc200ai_reset,
    .pfn_sensor_streaming_ctrl              = sc200ai_sensor_streaming_ctrl,
    .pfn_sensor_testpattern                 = sc200ai_testpattern_ctrl,

    .pfn_sensor_set_mode                    = sc200ai_sensor_set_mode,
    .pfn_sensor_get_mode                    = sc200ai_sensor_get_mode,
    .pfn_sensor_set_fps                     = sc200ai_set_fps,
    .pfn_sensor_set_slaveaddr               = sc200ai_set_slaveaddr,

    /* communication : register read/write */
    .pfn_sensor_set_bus_info                = sc200ai_set_bus_info,
    .pfn_sensor_write_register              = sc200ai_write_register,
    .pfn_sensor_read_register               = sc200ai_read_register,

    /* default param */
    .pfn_sensor_get_default_params          = sc200ai_get_isp_default_params,
    .pfn_sensor_get_black_level             = sc200ai_get_black_level,

    /* ae ctrl */
    .pfn_sensor_get_hw_exposure_params      = sc200ai_get_hw_exposure_params,
    .pfn_sensor_get_gain_table              = sc200ai_get_sensor_gain_table,
    .pfn_sensor_set_again                   = sc200ai_set_again,
    .pfn_sensor_set_dgain                   = sc200ai_set_dgain,
    .pfn_sensor_hcglcg_ctrl                 = AX_NULL,

    .pfn_sensor_set_integration_time        = sc200ai_set_integration_time,
    .pfn_sensor_get_integration_time_range  = sc200ai_get_integration_time_range,
    .pfn_sensor_set_slow_fps                = sc200ai_set_slow_fps,
    .pfn_sensor_get_slow_shutter_param      = sc200ai_get_slow_shutter_param,
    .pfn_sensor_get_sns_reg_info            = sc200ai_ae_get_sensor_reg_info,
};
