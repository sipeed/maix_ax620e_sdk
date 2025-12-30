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
#include "ax_base_type.h"
#include "ax_isp_common.h"

#include "drv_i2c.h"
#include "drv_gpio.h"
#include "isp_sensor_types.h"
#include "isp_sensor_internal.h"

#include "sc235hai_reg.h"
#include "sc235hai_settings.h"

extern SNS_STATE_OBJ *g_szsc235haiCtx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szsc235haiCtx[dev])

static AX_SNS_COMMBUS_T gsc235haiBusInfo[AX_VIN_MAX_PIPE_NUM] = {0};

AX_U8 gSc235haiSlaveAddr[AX_VIN_MAX_PIPE_NUM] = {SC235HAI_SLAVE_ADDR1, SC235HAI_SLAVE_ADDR2};

AX_S32 sc235hai_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo)
{
    gsc235haiBusInfo[nPipeId].I2cDev = tSnsBusInfo.I2cDev;

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_get_bus_num(ISP_PIPE_ID nPipeId)
{
    return gsc235haiBusInfo[nPipeId].I2cDev;
}

AX_S32 sc235hai_set_slaveaddr(ISP_PIPE_ID nPipeId, AX_U8 nslaveaddr)
{
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    if (nslaveaddr == SC235HAI_SLAVE_ADDR1 || nslaveaddr == SC235HAI_SLAVE_ADDR2)
        gSc235haiSlaveAddr[nPipeId] = nslaveaddr;
    else
        gSc235haiSlaveAddr[nPipeId] = SC235HAI_SLAVE_ADDR1;

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_sensor_i2c_init(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    char i2c_name[8] = {0};
    rt_device_t i2c_dev = RT_NULL;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sns_i2c_obj.i2c_dev = AX_NULL;
    sns_obj->sns_i2c_obj.slave_addr = gSc235haiSlaveAddr[nPipeId];
    sns_obj->sns_i2c_obj.address_byte = SC235HAI_ADDR_BYTE;
    sns_obj->sns_i2c_obj.data_byte = SC235HAI_DATA_BYTE;
    sns_obj->sns_i2c_obj.swap_byte = SC235HAI_SWAP_BYTES;

    sns_obj->sns_i2c_obj.sns_i2c_bnum = sc235hai_get_bus_num(nPipeId);
    rt_sprintf(i2c_name, "i2c%d", sns_obj->sns_i2c_obj.sns_i2c_bnum);

    i2c_dev = rt_device_find(i2c_name);
    if (!i2c_dev) {
        ret = i2c_init(sns_obj->sns_i2c_obj.sns_i2c_bnum, i2c_name, i2c_freq_400k);
        if (ret != 0) {
            SNS_ERR("i2c init error");
            return ret;
        }

        sns_obj->sns_i2c_obj.i2c_dev = (struct rt_i2c_bus_device*)(rt_device_find(i2c_name));
        if(sns_obj->sns_i2c_obj.i2c_dev == AX_NULL) {
            SNS_ERR("rt_device_find failed!\n");
            return SNS_ERR_CODE_FAILED;
        }
    }
    else {
        sns_obj->sns_i2c_obj.i2c_dev = (struct rt_i2c_bus_device*)i2c_dev;
    }

    SNS_DBG("sc200ai i2c init finish, i2c bus %d \n", sns_obj->sns_i2c_obj.sns_i2c_bnum);

    return SNS_SUCCESS;
}

AX_S32 sc235hai_sensor_i2c_exit(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    //ret = i2c_exit(sns_obj->sns_i2c_obj.sns_i2c_fd);

    return ret;
}

AX_S32 sc235hai_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 nRet = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (AX_NULL == sns_obj->sns_i2c_obj.i2c_dev)
        return -1;

    nRet = i2c_read_reg(
            sns_obj->sns_i2c_obj.i2c_dev,
            sns_obj->sns_i2c_obj.slave_addr,
            nAddr,
            (AX_U8 *)(pData),
            sns_obj->sns_i2c_obj.data_byte,
            1);

    if(nRet < 0) {
        SNS_ERR("sc200ai read reg failed, ret=%d\n", nRet);
    }

    return nRet;
}

AX_S32 sc235hai_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr)
{
    AX_U8 data;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    AX_S32 nRet = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (AX_NULL == sns_obj->sns_i2c_obj.i2c_dev)
        return -1;

    nRet = i2c_read_reg(
            sns_obj->sns_i2c_obj.i2c_dev,
            sns_obj->sns_i2c_obj.slave_addr,
            addr,
            (AX_U8 *)(&data),
            sns_obj->sns_i2c_obj.data_byte,
            1);

    if(nRet < 0) {
        SNS_ERR("sc200ai read reg failed, ret=%d\n", nRet);
    }
    return data;
}

AX_S32 sc235hai_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (AX_NULL == sns_obj->sns_i2c_obj.i2c_dev) {
        return -1;
    }

    nRet = i2c_wrtie_reg(
                sns_obj->sns_i2c_obj.i2c_dev,
                sns_obj->sns_i2c_obj.slave_addr,
                addr,
                (AX_U8 *)(&data),
                sns_obj->sns_i2c_obj.data_byte,
                1);

    if(nRet < 0) {
        SNS_ERR("sc200ai write reg failed, ret=%d\n", nRet);
    }

    return nRet;
}

AX_S32 sc235hai_hw_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio, AX_U8 nValue)
{
    AX_S32 result = 0;
    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    if (nValue == gpio_low) {
        gpio_set_mode(nResetGpio, gpio_mode_output);
    }

    gpio_set_value(nResetGpio, nValue);

    return result;
}
AX_S32 sc235hai_group_prepare(ISP_PIPE_ID nPipeId)
{
    AX_S32 result = 0;

    return result;
}

#if 0
AX_S32 sc235hai_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio)
{
    AX_S32 result = 0;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    result |= sc235hai_hw_reset(nResetGpio, 0);
    usleep(5);
    result |= sc235hai_hw_reset(nResetGpio, 1);
    usleep(5 * 1000);

    return result;
}
#endif

AX_U32 sc235hai_get_hts(ISP_PIPE_ID nPipeId)
{
    AX_U8 hts_l = 0;
    AX_U8 hts_h = 0;
    AX_U32 hts = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    hts_h = sc235hai_reg_read(nPipeId, SC235HAI_HTS_L_H);
    hts_l = sc235hai_reg_read(nPipeId, SC235HAI_HTS_L_L);

    hts = hts_h << 8 | hts_l;

    return hts;
}

AX_U32 sc235hai_set_hts(ISP_PIPE_ID nPipeId, AX_U32 hts)
{
    AX_U8 hts_l = 0;
    AX_U8 hts_h = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    hts_h = (hts >> 8) & 0xff;
    hts_l = hts & 0xff;
    sc235hai_write_register(nPipeId, SC235HAI_HTS_L_H, hts_h);
    sc235hai_write_register(nPipeId, SC235HAI_HTS_L_L, hts_l);

    return AX_SNS_SUCCESS;
}

AX_U32 sc235hai_get_vts(ISP_PIPE_ID nPipeId)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_U32 vts = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_h = sc235hai_reg_read(nPipeId, SC235HAI_VTS_L_H);
    vts_l = sc235hai_reg_read(nPipeId, SC235HAI_VTS_L_L);
    vts = (AX_U32)(((vts_h & 0xFF) << 8) | (AX_U32)(vts_l & 0xFF));

    return vts;
}

AX_U32 sc235hai_set_vts(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_h = (vts >> 8) & 0xff;
    vts_l = vts & 0xff;
    result |= sc235hai_write_register(nPipeId, SC235HAI_VTS_L_H, vts_h);
    result |= sc235hai_write_register(nPipeId, SC235HAI_VTS_L_L, vts_l);

    return result;
}

AX_U32 sc235hai_get_vs_vts(ISP_PIPE_ID nPipeId)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_U32 vts = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_h = sc235hai_reg_read(nPipeId, SC235HAI_VTS_S_H);
    vts_l = sc235hai_reg_read(nPipeId, SC235HAI_VTS_S_L);
    vts = (AX_U32)(((vts_h & 0xFF) << 8) | (AX_U32)(vts_l & 0xFF));

    return vts;
}

AX_U32 sc235hai_set_vts_s(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_h = (vts >> 8) & 0xff;
    vts_l = vts & 0xff;
    result |= sc235hai_write_register(nPipeId, SC235HAI_VTS_S_H, vts_h);
    result |= sc235hai_write_register(nPipeId, SC235HAI_VTS_S_L, vts_l);

    return result;
}

AX_F32 sc235hai_get_exp_offset(ISP_PIPE_ID nPipeId)
{
    AX_F32 offset = 0.0f;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    offset += (sc235hai_reg_read(nPipeId, 0x3301) & 0x3) * 2.0f;
    offset += sc235hai_reg_read(nPipeId, 0x3302);
    offset += (sc235hai_reg_read(nPipeId, 0x3303) & 0x1f);
    offset += sc235hai_reg_read(nPipeId, 0x3304);
    offset += ((sc235hai_reg_read(nPipeId, 0x3305) & 0xFF) << 8) |
              (sc235hai_reg_read(nPipeId, 0x3306) & 0xFF);
    offset += (sc235hai_reg_read(nPipeId, 0x3307) & 0x1f);
    offset += sc235hai_reg_read(nPipeId, 0x3308) * 2.0f;
    offset += sc235hai_reg_read(nPipeId, 0x330d);

    offset /= (AX_F32)sc235hai_get_hts(nPipeId);

    return offset;
}

AX_S32 sc235hai_get_vts_from_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array *setting, AX_U32 reg_cnt, AX_U32 *vts)
{
    AX_U32 i = 0;
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_U8 mask = 0;

    for (i = 0; i < reg_cnt; i++) {
        if ((setting + i)->addr == SC235HAI_VTS_L_H) {
            vts_h = (setting + i)->value;
            mask |= 1;
        } else if ((setting + i)->addr == SC235HAI_VTS_L_L) {
            vts_l = (setting + i)->value;
            mask |= 1 << 1;
        }

        if (mask == 0x3) break;
    }

    if (mask != 0x3) {
        SNS_ERR("get setting vts fail, mask:0x%x\n", mask);
        return AX_SNS_ERR_NOT_MATCH;
    }

    *vts = vts_h << 8 | vts_l;

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_select_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array **setting, AX_U32 *cnt)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    switch (sns_obj->eImgMode) {
    case e_SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS:
        *setting = SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS;
        *cnt = sizeof(SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC235HAI_MIPI_24M_396MBPS_2LANE_1920x1080_10BIT_SDR_30FPS:
        *setting = SC235HAI_MIPI_24M_396MBPS_2LANE_1920x1080_10BIT_SDR_30FPS;
        *cnt = sizeof(SC235HAI_MIPI_24M_396MBPS_2LANE_1920x1080_10BIT_SDR_30FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS:
        *setting = SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS;
        *cnt = sizeof(SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC235HAI_MIPI_27M_396MBPS_2LANE_1920x1080_10BIT_SDR_30FPS:
        *setting = SC235HAI_MIPI_27M_396MBPS_2LANE_1920x1080_10BIT_SDR_30FPS;
        *cnt = sizeof(SC235HAI_MIPI_27M_396MBPS_2LANE_1920x1080_10BIT_SDR_30FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC235HAI_MIPI_27M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS:
        *setting = SC235HAI_MIPI_27M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS;
        *cnt = sizeof(SC235HAI_MIPI_27M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS) / sizeof(camera_i2c_reg_array);
        break;
    default:
        SNS_ERR("it's not supported. pipe=%d, setting mode=%d\n", nPipeId, sns_obj->eImgMode);
        return AX_SNS_ERR_NOT_SUPPORT;
    }

    SNS_INFO("pipe=%d, setting mode=%d\n", nPipeId, sns_obj->eImgMode);

    return AX_SNS_SUCCESS;
}

AX_S32 sc235hai_write_settings(ISP_PIPE_ID nPipeId)
{
    AX_U32 i = 0;
    AX_S32 ret = 0;
    AX_U32 reg_cnt = 0;
    camera_i2c_reg_array *setting = AX_NULL;

    ret = sc235hai_select_setting(nPipeId, &setting, &reg_cnt);
    if (ret) {
        return ret;
    }

    for (i = 0; i < reg_cnt; i++) {
        sc235hai_write_register(nPipeId, (setting + i)->addr, ((setting + i)->value));
#ifdef SENSOR_DEBUG
        usleep(2 * 1000);
        AX_U8 val = sc235hai_reg_read(nPipeId, (default_setting + i)->addr);
        SNS_DBG(" addr: 0x%04x write:0x%02x read:0x%02x \n",
                (default_setting + i)->addr, (default_setting + i)->value, val);
#endif
    }

    return AX_SNS_SUCCESS;
}
