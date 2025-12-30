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

#include "sc200ai_reg.h"
#include "sc200ai_settings.h"
#include "sc200ai_ae_ctrl.h"

extern SNS_STATE_OBJ *g_szsc200aiCtx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szsc200aiCtx[dev])

static AX_SNS_COMMBUS_T gsc200aiBusInfo[AX_VIN_MAX_PIPE_NUM] = {0};
AX_U8 gSc200aiSlaveAddr[AX_VIN_MAX_PIPE_NUM] = {SC200AI_SLAVE_ADDR1, SC200AI_SLAVE_ADDR2};

AX_S32 sc200ai_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo)
{
    gsc200aiBusInfo[nPipeId].I2cDev = tSnsBusInfo.I2cDev;

    return SNS_SUCCESS;
}

AX_S32 sc200ai_get_bus_num(ISP_PIPE_ID nPipeId)
{
    return gsc200aiBusInfo[nPipeId].I2cDev;
}

AX_S32 sc200ai_set_slaveaddr(ISP_PIPE_ID nPipeId, AX_U8 nslaveaddr)
{
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    if (nslaveaddr == SC200AI_SLAVE_ADDR1 || nslaveaddr == SC200AI_SLAVE_ADDR2)
        gSc200aiSlaveAddr[nPipeId] = nslaveaddr;
    else
        gSc200aiSlaveAddr[nPipeId] = SC200AI_SLAVE_ADDR1;

    return SNS_SUCCESS;
}

AX_S32 sc200ai_sensor_i2c_init(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    char i2c_name[8] = {0};
    rt_device_t i2c_dev = RT_NULL;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sns_i2c_obj.i2c_dev = AX_NULL;
    sns_obj->sns_i2c_obj.slave_addr = gSc200aiSlaveAddr[nPipeId];
    sns_obj->sns_i2c_obj.address_byte = SC200AI_ADDR_BYTE;
    sns_obj->sns_i2c_obj.data_byte = SC200AI_DATA_BYTE;
    sns_obj->sns_i2c_obj.swap_byte = SC200AI_SWAP_BYTES;

    sns_obj->sns_i2c_obj.sns_i2c_bnum = sc200ai_get_bus_num(nPipeId);
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

AX_S32 sc200ai_sensor_i2c_exit(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    // ret = i2c_exit(sns_obj->sns_i2c_obj.sns_i2c_fd);

    return ret;
}

AX_S32 sc200ai_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData)
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

AX_S32 sc200ai_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr)
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

AX_S32 sc200ai_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data)
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

#if 0
AX_S32 sc200ai_hw_reset(unsigned int gpio_num, unsigned int gpio_out_val)
{
    FILE *fp = AX_NULL;
    char file_name[50];
    char buf[10];

    sprintf(file_name, "/sys/class/gpio/gpio%d", gpio_num);
    if (0 != access(file_name, F_OK)) {
        sprintf(file_name, "/sys/class/gpio/export");
        fp = fopen(file_name, "w");
        if (fp == AX_NULL) {
            SNS_ERR("Cannot open %s.\n", file_name);
            return -1;
        }
        fprintf(fp, "%d", gpio_num);
        fclose(fp);

        sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio_num);
        fp = fopen(file_name, "w");
        if (fp == AX_NULL) {
            SNS_ERR("Cannot open %s.\n", file_name);
            return -1;
        }
        fprintf(fp, "out");
        fclose(fp);
    }

    sprintf(file_name, "/sys/class/gpio/gpio%d/value", gpio_num);
    fp = fopen(file_name, "w");
    if (fp == AX_NULL) {
        SNS_ERR("Cannot open %s.\n", file_name);
        return -1;
    }
    if (gpio_out_val) {
        strcpy(buf, "1");
    } else {
        strcpy(buf, "0");
    }
    fprintf(fp, "%s", buf);
    fclose(fp);

    return 0;
}
#endif
AX_S32 sc200ai_group_prepare(ISP_PIPE_ID nPipeId)
{
    AX_S32 result = 0;

    return result;
}


AX_S32 sc200ai_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio, AX_U8 nValue)
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

AX_U32 sc200ai_get_vts_s(ISP_PIPE_ID nPipeId)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_U32 vts = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_h = sc200ai_reg_read(nPipeId, SC200AI_VTS_SHORT_H);
    vts_l = sc200ai_reg_read(nPipeId, SC200AI_VTS_SHORT_L);

    vts = (vts_h & 0x7f) << 8 | vts_l;

    return vts;
}

AX_U32 sc200ai_set_vts_s(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_h = (vts >> 8) & 0x7f;
    vts_l = vts & 0xff;

    result |= sc200ai_update_regidx_table(nPipeId, SC200AI_VTS_SHORT_H_IDX, vts_h);
    result |= sc200ai_update_regidx_table(nPipeId, SC200AI_VTS_SHORT_L_IDX, vts_l);

    return result;
}
#if 0
AX_U32 sc200ai_get_hts(ISP_PIPE_ID nPipeId)
{
    AX_U8 hts_l = 0;
    AX_U8 hts_h = 0;
    AX_U32 hts = 0;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    hts_h = sc200ai_reg_read(nPipeId, SC200AI_HTS_L_H);
    hts_l = sc200ai_reg_read(nPipeId, SC200AI_HTS_L_L);

    hts = hts_h << 8 | hts_l;

    return hts;
}

AX_U32 sc200ai_set_hts(ISP_PIPE_ID nPipeId, AX_U32 hts)
{
    AX_U8 hts_l = 0;
    AX_U8 hts_h = 0;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    hts_h = (hts >> 8) & 0xff;
    hts_l = hts & 0xff;
    sc200ai_write_register(nPipeId, SC200AI_HTS_L_H, hts_h);
    sc200ai_write_register(nPipeId, SC200AI_HTS_L_L, hts_l);

    return 0;
}
#endif
AX_U32 sc200ai_get_vts(ISP_PIPE_ID nPipeId)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_U32 vts = 0;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    vts_h = sc200ai_reg_read(nPipeId, SC200AI_VTS_LONG_H);
    vts_l = sc200ai_reg_read(nPipeId, SC200AI_VTS_LONG_L);

    vts = (AX_U32)(((vts_h & 0xFF) << 8) | (AX_U32)(vts_l & 0xFF));

    return vts;
}

AX_U32 sc200ai_set_vts(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_S32 result = 0;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    vts_h = (vts >> 8) & 0xff;
    vts_l = vts & 0xff;

    result |= sc200ai_update_regidx_table(nPipeId, SC200AI_VTS_LONG_H_IDX, vts_h);
    result |= sc200ai_update_regidx_table(nPipeId, SC200AI_VTS_LONG_L_IDX, vts_l);

    return result;
}

AX_U32 sc200ai_get_vs_vts(ISP_PIPE_ID nPipeId)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_U32 vts = 0;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    vts_h = sc200ai_reg_read(nPipeId, SC200AI_VTS_SHORT_H);
    vts_l = sc200ai_reg_read(nPipeId, SC200AI_VTS_SHORT_L);

    vts = (AX_U32)(((vts_h & 0xFF) << 8) | (AX_U32)(vts_l & 0xFF));

    return vts;
}

AX_U32 sc200ai_set_vs_vts(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vts_h = 0;
    AX_U8 vts_l = 0;
    AX_S32 result = 0;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    vts_h = (vts >> 8) & 0xff;
    vts_l = vts & 0xff;

    result |= sc200ai_update_regidx_table(nPipeId, SC200AI_VTS_SHORT_H_IDX, vts_h);
    result |= sc200ai_update_regidx_table(nPipeId, SC200AI_VTS_SHORT_L_IDX, vts_l);

    return result;
}

AX_S32 sc200ai_write_settings(ISP_PIPE_ID nPipeId, AX_U32 setindex)
{
    AX_S32 i;
    AX_S32 reg_count = 0;
    const camera_i2c_reg_array *default_setting = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SNS_DBG("sc200ai setitng index: %d\n", setindex);

    switch (setindex) {
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS:
        default_setting = &SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS:
        default_setting = &SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_MASTER:
        default_setting = &SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_MASTER[0];
        reg_count = sizeof(SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_MASTER) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_SLAVE:
        default_setting = &SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_SLAVE[0];
        reg_count = sizeof(SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_SLAVE) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_MASTER:
        default_setting = &SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_MASTER[0];
        reg_count = sizeof(SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_MASTER) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_SLAVE:
        default_setting = &SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_SLAVE[0];
        reg_count = sizeof(SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_SLAVE) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_MASTER:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_MASTER[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_MASTER) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_SLAVE:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_SLAVE[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_SLAVE) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_MASTER:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_MASTER[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_MASTER) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_SLAVE:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_SLAVE[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_SLAVE) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_25FPS:
        default_setting = &SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_25FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_25FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_30FPS:
        default_setting = &SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_30FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_30FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_25FPS:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_25FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_25FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_30FPS:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_30FPS[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_30FPS) / sizeof(camera_i2c_reg_array);
        break;
    case e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS_SYNC_SLAVE:
        default_setting = &SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS_SYNC_SLAVE[0];
        reg_count = sizeof(SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS_SYNC_SLAVE) / sizeof(camera_i2c_reg_array);
        break;
    default:
        SNS_ERR("it's not supported. pipe=%d, setting mode=%d] \n", nPipeId, setindex);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    SNS_DBG("sc200ai setitng index: %d, reg_count %d\n", setindex, reg_count);
    for (i = 0; i < reg_count; i++) {
        sc200ai_write_register(nPipeId, (default_setting + i)->addr, ((default_setting + i)->value));
#ifdef SENSOR_DEBUG
        rt_thread_mdelay(2);
        rBuf[0] = sc200ai_reg_read(nPipeId, (default_setting + i)->addr);
        SNS_DBG(" addr: 0x%04x write:0x%02x read:0x%02x \r\n",
                (default_setting + i)->addr, (default_setting + i)->value, rBuf[0]);
#endif
    }

    return 0;
}
