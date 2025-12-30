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

#include "os04a10_reg.h"
#include "os04a10_settings.h"

extern SNS_STATE_OBJ *g_szOs04a10Ctx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs04a10Ctx[dev])

static AX_SNS_COMMBUS_T gOs04A10BusInfo[AX_VIN_MAX_PIPE_NUM] = { {0}, {2},};

AX_U8 gOs04A10SlaveAddr[AX_VIN_MAX_PIPE_NUM] = {0x36, 0x36};

AX_S32 os04a10_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo)
{
    gOs04A10BusInfo[nPipeId].I2cDev = tSnsBusInfo.I2cDev;

    return SNS_SUCCESS;
}

AX_S32 os04a10_get_bus_num(ISP_PIPE_ID nPipeId)
{
    return gOs04A10BusInfo[nPipeId].I2cDev;
}

AX_S32 os04a10_set_slaveaddr(ISP_PIPE_ID nPipeId, AX_U8 nslaveaddr)
{
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    if (nslaveaddr == OS04A10_SLAVE_ADDR1 || nslaveaddr == OS04A10_SLAVE_ADDR2)
        gOs04A10SlaveAddr[nPipeId] = nslaveaddr;
    else
        gOs04A10SlaveAddr[nPipeId] = OS04A10_SLAVE_ADDR1;

    return SNS_SUCCESS;
}

AX_S32 os04a10_sensor_i2c_init(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    char i2c_name[8] = {0};
    rt_device_t i2c_dev = RT_NULL;
    SNS_STATE_OBJ *sns_obj = RT_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sns_i2c_obj.i2c_dev = RT_NULL;
    sns_obj->sns_i2c_obj.slave_addr = gOs04A10SlaveAddr[nPipeId];
    sns_obj->sns_i2c_obj.address_byte = OS04A10_ADDR_BYTE;
    sns_obj->sns_i2c_obj.data_byte = OS04A10_DATA_BYTE;
    sns_obj->sns_i2c_obj.swap_byte = OS04A10_SWAP_BYTES;

    sns_obj->sns_i2c_obj.sns_i2c_bnum = os04a10_get_bus_num(nPipeId);
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

    SNS_DBG("os04a10 i2c init finish, i2c bus %d \n", sns_obj->sns_i2c_obj.sns_i2c_bnum);

    return SNS_SUCCESS;
}

AX_S32 os04a10_sensor_i2c_exit(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    SNS_STATE_OBJ *sns_obj = RT_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    //i2c_exit(sns_obj->sns_i2c_obj.sns_i2c_fd);

    return ret;
}

AX_S32 os04a10_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData)
{
    //AX_U8 data;
    SNS_STATE_OBJ *sns_obj = RT_NULL;
    AX_S32 nRet = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (RT_NULL == sns_obj->sns_i2c_obj.i2c_dev)
        return -1;

    nRet = i2c_read_reg(
            sns_obj->sns_i2c_obj.i2c_dev,
            sns_obj->sns_i2c_obj.slave_addr,
            nAddr,
            (AX_U8 *)(pData),
            sns_obj->sns_i2c_obj.data_byte,
            1);

    if(nRet < 0) {
        SNS_ERR("os04a10 read reg failed, ret=%d\n", nRet);
    }

    return nRet;
}

AX_S32 os04a10_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr)
{
    AX_U8 data;
    SNS_STATE_OBJ *sns_obj = RT_NULL;
    AX_S32 nRet = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (RT_NULL == sns_obj->sns_i2c_obj.i2c_dev)
        return -1;

    nRet = i2c_read_reg(
            sns_obj->sns_i2c_obj.i2c_dev,
            sns_obj->sns_i2c_obj.slave_addr,
            addr,
            (AX_U8 *)(&data),
            sns_obj->sns_i2c_obj.data_byte,
            1);

    if(nRet < 0) {
        SNS_ERR("os04a10 read reg failed, ret=%d\n", nRet);
    }

    return data;
}

AX_S32 os04a10_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = RT_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (RT_NULL == sns_obj->sns_i2c_obj.i2c_dev) {
        return -1;
    }

    nRet = i2c_wrtie_reg(
                sns_obj->sns_i2c_obj.i2c_dev,
                sns_obj->sns_i2c_obj.slave_addr,
                addr,
                (AX_U8 *)(&data),
                sns_obj->sns_i2c_obj.data_byte,
                1);

    return nRet;
}

#if 0
AX_S32 os04a10_hw_reset(unsigned int gpio_num, unsigned int gpio_out_val)
{
    FILE *fp = NULL;
    char file_name[50];
    char buf[10];

    sprintf(file_name, "/sys/class/gpio/gpio%d", gpio_num);
    if (0 != access(file_name, F_OK)) {
        sprintf(file_name, "/sys/class/gpio/export");
        fp = fopen(file_name, "w");
        if (fp == NULL) {
            SNS_ERR("Cannot open %s.\n", file_name);
            return -1;
        }
        fprintf(fp, "%d", gpio_num);
        fclose(fp);

        sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio_num);
        fp = fopen(file_name, "w");
        if (fp == NULL) {
            SNS_ERR("Cannot open %s.\n", file_name);
            return -1;
        }
        fprintf(fp, "out");
        fclose(fp);
    }

    sprintf(file_name, "/sys/class/gpio/gpio%d/value", gpio_num);
    fp = fopen(file_name, "w");
    if (fp == NULL) {
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
AX_S32 os04a10_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio, AX_U8 nValue)
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

AX_S32 os04a10_group_prepare(void)
{
    AX_S32 result = 0;

    result |= os04a10_write_register(0, 0x320D, 0x00);
    result |= os04a10_write_register(0, 0x3208, 0x00);
    result |= os04a10_write_register(0, 0x0808, 0x00);
    result |= os04a10_write_register(0, 0x3800, 0x11);
    result |= os04a10_write_register(0, 0x3911, 0x22);
    result |= os04a10_write_register(0, 0x3208, 0x10);

    return result;
}


AX_U32 os04a10_get_hts(ISP_PIPE_ID nPipeId)
{
    AX_U8 hts_h;
    AX_U8 hts_l;
    AX_U32 hts;

    if (nPipeId < 0 || (nPipeId >= AX_VIN_MAX_PIPE_NUM))
        return -1;

    hts_h = os04a10_reg_read(nPipeId, 0x380C);
    hts_l = os04a10_reg_read(nPipeId, 0x380D);

    hts = (AX_U32)(((hts_h & 0xF) << 8) | (AX_U32)(hts_l << 0));

    return hts;
}

AX_U32 os04a10_get_vs_hts(ISP_PIPE_ID nPipeId)
{
    AX_U8 hts_h;
    AX_U8 hts_l;
    AX_U32 hts;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    hts_h = os04a10_reg_read(nPipeId, 0x384C);
    hts_l = os04a10_reg_read(nPipeId, 0x384D);

    hts = (AX_U32)(((hts_h & 0xFF) << 8) | (AX_U32)(hts_l << 0));

    return hts;
}

AX_U32 os04a10_set_hts(ISP_PIPE_ID nPipeId, AX_U32 hts)
{
    AX_U8 hts_h;
    AX_U8 hts_l;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    hts_l = hts & 0xFF;
    hts_h = (hts & 0xFF00) >> 8;

    result |= os04a10_write_register(nPipeId, 0x380C, hts_h);
    result |= os04a10_write_register(nPipeId, 0x380D, hts_l);

    return result;
}

AX_U32 os04a10_get_vts(ISP_PIPE_ID nPipeId)
{
    AX_U8 vts_h;
    AX_U8 vts_l;
    AX_U32 vts;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vts_h = os04a10_reg_read(nPipeId, OS04A10_VTS_H);
    vts_l = os04a10_reg_read(nPipeId, OS04A10_VTS_L);

    vts = (AX_U32)(((vts_h & 0xFF) << 8) | (AX_U32)(vts_l << 0));

    return vts;
}

AX_F32 os04a10_get_sclk(ISP_PIPE_ID nPipeId)
{
    AX_U8 pre_div0;
    AX_U8 pre_div;
    AX_U16 multiplier;
    AX_U8 post_div;
    //AX_U8 sram_div;
    AX_U8 st_div;
    AX_U8 t_div;
    float inck;
    float sclk;

    inck = OS04A10_INCK_24M;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    pre_div0 = (os04a10_reg_read(nPipeId, 0x0322) & 0x1) + 1;

    pre_div = os04a10_reg_read(nPipeId, 0x0323) & 0x7;
    if (pre_div == 0) {
        pre_div = 1;
    } else if (pre_div == 1) {
        pre_div = 1.5;
    } else if (pre_div == 2) {
        pre_div = 2;
    } else if (pre_div == 3) {
        pre_div = 2.5;
    } else if (pre_div == 4) {
        pre_div = 3;
    } else if (pre_div == 5) {
        pre_div = 4;
    } else if (pre_div == 6) {
        pre_div = 6;
    } else if (pre_div == 7) {
        pre_div = 8;
    } else {
    }

    multiplier = (os04a10_reg_read(nPipeId, 0x0324) & 0x3) << 8;
    multiplier = multiplier | ((os04a10_reg_read(nPipeId, 0x0325)) & 0xFF);

    post_div = (os04a10_reg_read(nPipeId, 0x032f) & 0xF) + 1;
    //sram_div = (os04a10_reg_read(nPipeId, 0x0327) & 0xF) + 1;
    st_div = (os04a10_reg_read(nPipeId, 0x0328) & 0xF) + 1;

    t_div = os04a10_reg_read(nPipeId, 0x032a) & 0x7;
    if (t_div == 0) {
        t_div = 1;
    } else if (t_div == 1) {
        t_div = 1.5;
    } else if (t_div == 2) {
        t_div = 2;
    } else if (t_div == 3) {
        t_div = 2.5;
    } else if (t_div == 4) {
        t_div = 3;
    } else if (t_div == 5) {
        t_div = 3.5;
    } else if (t_div == 6) {
        t_div = 4;
    } else if (t_div == 7) {
        t_div = 5;
    } else {
    }
    sclk = (((((((float)(inck * 1000 * 1000) / pre_div0) / pre_div) * multiplier) / post_div) / st_div) / t_div);
    /*SNS_DBG("%s pre_div0=0x%x, pre_div=0x%x, multiplier=0x%x, post_div=0x%x, sram_div=0x%x, st_div=0x%x, t_div=0x%x \n", \
          __func__, pre_div0, pre_div, multiplier, post_div, sram_div, st_div, t_div); */

    return sclk;
}

AX_S32 os04a10_write_settings(ISP_PIPE_ID nPipeId, AX_U32 setindex)
{
    AX_S32 i = 0;
    //AX_S32 errnum = 0;
    //AX_U8 rBuf[1];
    AX_S32 reg_count = 0;
    const camera_i2c_reg_array *default_setting = RT_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SNS_DBG("os04a10 setitng index: %d\n", setindex);

    switch (setindex) {
    /* 4 lane */
    case e_OS04A10_4lane_2688x1520_10bit_Linear_60fps:
        default_setting = &OS04A10_4lane_2688x1520_10bit_Linear_60fps[0];
        reg_count = sizeof(OS04A10_4lane_2688x1520_10bit_Linear_60fps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS04A10_4lane_2688x1520_10bit_Linear_30fps:
        default_setting = &OS04A10_4lane_2688x1520_10bit_Linear_30fps[0];
        reg_count = sizeof(OS04A10_4lane_2688x1520_10bit_Linear_30fps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS04A10_4lane_2688x1520_12bit_Linear_30fps:
        default_setting = &OS04A10_4lane_2688x1520_12bit_Linear_30fps[0];
        reg_count = sizeof(OS04A10_4lane_2688x1520_12bit_Linear_30fps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS04A10_4lane_2688x1520_10bit_2Stagger_HDR_30fps:
        default_setting = &OS04A10_4lane_2688x1520_10bit_2Stagger_HDR_30fps[0];
        reg_count = sizeof(OS04A10_4lane_2688x1520_10bit_2Stagger_HDR_30fps) / sizeof(camera_i2c_reg_array);
        break;

    /* 2 lane */
    case e_OS04A10_2lane_2688x1520_10bit_Linear_30fps:
        default_setting = &OS04A10_2lane_2688x1520_10bit_Linear_30fps[0];
        reg_count = sizeof(OS04A10_2lane_2688x1520_10bit_Linear_30fps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS04A10_2lane_2688x1520_10bit_2Stagger_HDR_30fps:
        default_setting = &OS04A10_2lane_2688x1520_10bit_2Stagger_HDR_30fps[0];
        reg_count = sizeof(OS04A10_2lane_2688x1520_10bit_2Stagger_HDR_30fps) / sizeof(camera_i2c_reg_array);
        break;

    case e_OS04A10_2lane_2688x1520_10bit_Linear_60fps:
        default_setting = &OS04A10_2lane_2688x1520_10bit_Linear_60fps[0];
        reg_count = sizeof(OS04A10_2lane_2688x1520_10bit_Linear_60fps) / sizeof(camera_i2c_reg_array);
        break;

    default:
        SNS_ERR("it's not supported. pipe=%d, setting mode=%d] \n", nPipeId, setindex);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }

    SNS_DBG("os04a10 setitng index: %d, reg_count %d\n", setindex, reg_count);
    for (i = 0; i < reg_count; i++) {
        os04a10_write_register(nPipeId, (default_setting + i)->addr, ((default_setting + i)->value));
#ifdef SENSOR_DEBUG
        rt_thread_mdelay(2);

        rBuf[0] = os04a10_reg_read(nPipeId, (default_setting + i)->addr);
        SNS_DBG(" addr: 0x%04x write:0x%02x read:0x%02x \r\n",
                (default_setting + i)->addr, default_setting + i)->value, rBuf[0]);
#endif
    }

    return 0;
}
