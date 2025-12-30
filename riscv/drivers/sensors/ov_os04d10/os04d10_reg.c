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

#include "os04d10_reg.h"
#include "os04d10_settings.h"
#include "os04d10_ae_ctrl.h"

extern SNS_STATE_OBJ *g_szOs04d10Ctx[AX_VIN_MAX_PIPE_NUM];
#define SENSOR_GET_CTX(dev, pstCtx) (pstCtx = g_szOs04d10Ctx[dev])

static AX_SNS_COMMBUS_T gOs04D10BusInfo[AX_VIN_MAX_PIPE_NUM] = { {0}, {2},};

AX_U8 gOs04D10SlaveAddr[AX_VIN_MAX_PIPE_NUM] = {0x3C, 0x3C};

AX_S32 os04d10_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo)
{
    gOs04D10BusInfo[nPipeId].I2cDev = tSnsBusInfo.I2cDev;

    return SNS_SUCCESS;
}

AX_S32 os04d10_get_bus_num(ISP_PIPE_ID nPipeId)
{
    return gOs04D10BusInfo[nPipeId].I2cDev;
}

AX_S32 os04d10_set_slaveaddr(ISP_PIPE_ID nPipeId, AX_U8 nslaveaddr)
{
    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    if (nslaveaddr == OS04D10_SLAVE_ADDR1 || nslaveaddr == OS04D10_SLAVE_ADDR2)
        gOs04D10SlaveAddr[nPipeId] = nslaveaddr;
    else
        gOs04D10SlaveAddr[nPipeId] = OS04D10_SLAVE_ADDR1;

    return SNS_SUCCESS;
}

AX_S32 os04d10_sensor_i2c_init(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    char i2c_name[8] = {0};
    rt_device_t i2c_dev = RT_NULL;
    SNS_STATE_OBJ *sns_obj = RT_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    sns_obj->sns_i2c_obj.i2c_dev = RT_NULL;
    sns_obj->sns_i2c_obj.slave_addr = gOs04D10SlaveAddr[nPipeId];
    sns_obj->sns_i2c_obj.address_byte = OS04D10_ADDR_BYTE;
    sns_obj->sns_i2c_obj.data_byte = OS04D10_DATA_BYTE;
    sns_obj->sns_i2c_obj.swap_byte = OS04D10_SWAP_BYTES;

    sns_obj->sns_i2c_obj.sns_i2c_bnum = os04d10_get_bus_num(nPipeId);
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

    SNS_DBG("os04d10 i2c init finish, i2c bus %d \n", sns_obj->sns_i2c_obj.sns_i2c_bnum);

    return SNS_SUCCESS;
}

AX_S32 os04d10_sensor_i2c_exit(ISP_PIPE_ID nPipeId)
{
    AX_S32 ret = 0;
    SNS_STATE_OBJ *sns_obj = RT_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    //i2c_exit(sns_obj->sns_i2c_obj.sns_i2c_fd);

    return ret;
}

AX_S32 os04d10_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;
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
            0);

    if(nRet < 0) {
        SNS_ERR("os04d10 read reg failed, ret=%d\n", nRet);
    }

    return nRet;
}

AX_S32 os04d10_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr)
{
    AX_U8 data;
    SNS_STATE_OBJ *sns_obj = AX_NULL;
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
            0);

    if(nRet < 0) {
        SNS_ERR("os04d10 read reg failed, ret=%d\n", nRet);
    }

    return data;
}

AX_S32 os04d10_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data)
{
    AX_S32 nRet = 0;
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    if (RT_NULL == sns_obj->sns_i2c_obj.i2c_dev)
        return -1;

    nRet = i2c_wrtie_reg(
                sns_obj->sns_i2c_obj.i2c_dev,
                sns_obj->sns_i2c_obj.slave_addr,
                addr,
                (AX_U8 *)(&data),
                sns_obj->sns_i2c_obj.data_byte,
                0);

    return nRet;
}

#if 0
AX_S32 os04d10_hw_reset(unsigned int gpio_num, unsigned int gpio_out_val)
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
            return SNS_ERR_CODE_ILLEGAL_PARAMS;
        }
        fprintf(fp, "%d", gpio_num);
        fclose(fp);

        sprintf(file_name, "/sys/class/gpio/gpio%d/direction", gpio_num);
        fp = fopen(file_name, "w");
        if (fp == NULL) {
            SNS_ERR("Cannot open %s.\n", file_name);
            return SNS_ERR_CODE_ILLEGAL_PARAMS;
        }
        fprintf(fp, "out");
        fclose(fp);
    }

    sprintf(file_name, "/sys/class/gpio/gpio%d/value", gpio_num);
    fp = fopen(file_name, "w");
    if (fp == NULL) {
        SNS_ERR("Cannot open %s.\n", file_name);
        return SNS_ERR_CODE_ILLEGAL_PARAMS;
    }
    if (gpio_out_val) {
        strcpy(buf, "1");
    } else {
        strcpy(buf, "0");
    }
    fprintf(fp, "%s", buf);
    fclose(fp);

    return SNS_SUCCESS;
}
#endif
AX_S32 os04d10_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio, AX_U8 nValue)
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

AX_U32 os04d10_get_hts(ISP_PIPE_ID nPipeId)
{
    AX_U8 hts_h;
    AX_U8 hts_l;
    AX_U32 hts;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    os04d10_write_register(nPipeId, OS04D10_PAGE_FLG_D2, OS04D10_PAGE_FLG_D2_P1);
    hts_h = os04d10_reg_read(nPipeId, OS04D10_HTS_H);
    hts_l = os04d10_reg_read(nPipeId, OS04D10_HTS_L);

    hts = (AX_U32)(((hts_h & 0xF) << 8) | (AX_U32)(hts_l << 0));

    return hts;
}

AX_U32 os04d10_get_vts(ISP_PIPE_ID nPipeId)
{
    AX_U32 vts;
    AX_U32 vblank;
    AX_U8 vblank_h;
    AX_U8 vblank_l;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    os04d10_write_register(nPipeId, OS04D10_PAGE_FLG_D2, OS04D10_PAGE_FLG_D2_P1);
    vblank_h = os04d10_reg_read(nPipeId, OS04D10_VBLANK_H);
    vblank_l = os04d10_reg_read(nPipeId, OS04D10_VBLANK_L);

    vblank = (AX_U32)(((vblank_h & 0xFF) << 8) | (AX_U32)(vblank_l << 0));

    /* The os04d10 vts register is read-only and needs to be indirectly adjusted through vblank register*/
    vts = OS04D10_VTS_DEFAULT + vblank;

    return vts;
}

AX_U32 os04d10_set_vts(ISP_PIPE_ID nPipeId, AX_U32 vts)
{
    AX_U8 vblank_h;
    AX_U8 vblank_l;
    AX_S32 result = 0;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    vblank_l = vts & 0xFF;
    vblank_h = (vts & 0xFF00) >> 8;

    result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_PAGE_FLG_D2, OS04D10_PAGE_FLG_D2_P1);
    result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_VBLANK_H_IDX, vblank_h);
    result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_VBLANK_L_IDX, vblank_l);
    result |= os04d10_sns_update_regidx_table(nPipeId, OS04D10_VBLANK_GAIN_EXP_EN_IDX, 0x01);

    return result;
}

AX_S32 os04d10_get_vblank_from_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array *setting, AX_U32 reg_cnt, AX_U32 *vblank)
{
    AX_U32 i = 0;
    AX_U8 low = 0;
    AX_U8 high = 0;
    AX_U8 mask = 0;
    AX_U8 cur_page_flg = 0;

    for (i = 0; i < reg_cnt; i++) {
        if ((setting + i)->addr == OS04D10_PAGE_FLG_D2 && (setting + i)->value == OS04D10_PAGE_FLG_D2_P1) {
            cur_page_flg = OS04D10_PAGE_FLG_D2_P1;
            continue;
        } else if ((setting + i)->addr == OS04D10_PAGE_FLG_D2 && (setting + i)->value != OS04D10_PAGE_FLG_D2_P1) {
            cur_page_flg = 0x00;
            continue;
        }

        if (cur_page_flg == OS04D10_PAGE_FLG_D2_P1) {
            if ((setting + i)->addr == OS04D10_VBLANK_H) {
                high = (setting + i)->value;
                mask |= 1;
            } else if ((setting + i)->addr == OS04D10_VBLANK_L) {
                low = (setting + i)->value;
                mask |= 1 << 1;
            }
        }

        if (mask == 0x3) break;
    }

    //default value
    if ((mask & 0x1) == 0) {
        high = 0x00;
        mask |= 1;
    }
    if ((mask & (1 << 1)) == 0) {
        low = 0x00;
        mask |= 1 << 1;
    }

    if (mask != 0x3) {
        SNS_ERR("get setting vts fail, mask:0x%x\n", mask);
        return -1;
    }

    *vblank = (high << 8 | low);

    return SNS_SUCCESS;
}

AX_S32 os04d10_get_hts_from_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array *setting, AX_U32 reg_cnt, AX_U32 *vts)
{
    AX_U32 i = 0;
    AX_U8 low = 0;
    AX_U8 high = 0;
    AX_U8 mask = 0;
    AX_U8 cur_page_flg = 0;

    for (i = 0; i < reg_cnt; i++) {
        if ((setting + i)->addr == OS04D10_PAGE_FLG_D2 && (setting + i)->value == OS04D10_PAGE_FLG_D2_P1) {
            cur_page_flg = OS04D10_PAGE_FLG_D2_P1;
            continue;
        } else if ((setting + i)->addr == OS04D10_PAGE_FLG_D2 && (setting + i)->value != OS04D10_PAGE_FLG_D2_P1) {
            cur_page_flg = 0x00;
            continue;
        }

        if (cur_page_flg == OS04D10_PAGE_FLG_D2_P1) {
            if ((setting + i)->addr == OS04D10_HBLANK_H) {
                high = (setting + i)->value;
                mask |= 1;
            } else if ((setting + i)->addr == OS04D10_HBLANK_L) {
                low = (setting + i)->value;
                mask |= 1 << 1;
            }
        }

        if (mask == 0x3) break;
    }

    //default value
    if ((mask & 0x1) == 0) {
        high = 0x00;
        mask |= 1;
    }
    if ((mask & (1 << 1)) == 0) {
        low = 0x02;
        mask |= 1 << 1;
    }

    if (mask != 0x3) {
        SNS_ERR("get setting vts fail, mask:0x%x\n", mask);
        return -1;
    }

    *vts = (high << 8 | low) + OS04D10_HTS_DEFAULT;

    return SNS_SUCCESS;
}

AX_S32 os04d10_get_sclk_from_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array *setting, AX_U32 reg_cnt, AX_F32 *sclk)
{
    AX_U8 clk_sel = 0;           //P0:0x31[5]
    AX_U16 dpll_nc = 0;
    AX_U8 dpll_nc_h = 0;         //P0:0x2d[5:4]
    AX_U8 dpll_nc_l = 0;         //P0:0x41[7:0]
    AX_U8 dpll_predivp_sel = 0;  //P0:0x30[2]+1
    AX_U8 dpll_mc = 0;           //P0:0x42[2:0]
    AX_U8 cnt_clk_sel = 0;       //P0:0x30[2:1]+3
    AX_U8 row_clk_sel = 0;       //P0:0x45[5:4]+5
    AX_U8 mpll_mc = 0;           //P0:0x2f[2:0]
    AX_U16 mpll_nc = 0;
    AX_U8 mpll_nc_h = 0;         //P0:0x2d[1:0]
    AX_U8 mpll_nc_l = 0;         //P0:0x2e[7:0]
    AX_U8 mpll_predivp_sel = 0;  //P0:0x30[0]+1
    AX_U8 mpll_postdiv_sel = 0;  //P0:0x35[1:0]+1
    AX_U8 pclkdiv_sel = 0;       //P0:0x43[2:1]+3
    AX_U8 timerclk_ctrl = 0;     //P0:0x31[0]+1
    AX_U8 rowclk_ctrl = 0;       //P0:0x31[4]+1
    AX_U8 cur_page_flg = 1;
    float inck = 1.0f;

    AX_U32 i = 0;
    AX_U16 mask = 0;

    inck = OS04D10_INCK_24M;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);

    for (i = 0; i < reg_cnt; i++) {
        if ((setting + i)->addr == OS04D10_PAGE_FLG_D2 && (setting + i)->value == OS04D10_PAGE_FLG_D2_P0) {
            cur_page_flg = OS04D10_PAGE_FLG_D2_P0;
            continue;
        } else if ((setting + i)->addr == OS04D10_PAGE_FLG_D2 && (setting + i)->value != OS04D10_PAGE_FLG_D2_P0) {
            cur_page_flg = 0x01;
            continue;
        }

        if (cur_page_flg == OS04D10_PAGE_FLG_D2_P0) {
            if ((setting + i)->addr == 0x31) {
                clk_sel = (((setting + i)->value) & 0x20) >> 5;
                timerclk_ctrl = (((setting + i)->value) & 0x1) + 1;
                rowclk_ctrl = ((((setting + i)->value) & 0x10) >> 4) + 1;
                mask |= 1;
            } else if ((setting + i)->addr == 0x45) {
                row_clk_sel = ((((setting + i)->value) & 0x30) >> 4) + 5;
                mask |= 1 << 1;
            } else if ((setting + i)->addr == 0x2d) {
                dpll_nc_h = (((setting + i)->value) & 0x30) >> 4;
                mpll_nc_h = ((setting + i)->value) & 0x3;
                mask |= 1 << 2;
            } else if ((setting + i)->addr == 0x41) {
                dpll_nc_l = (setting + i)->value;
                mask |= 1 << 3;
            } else if ((setting + i)->addr == 0x2e) {
                mpll_nc_l = (setting + i)->value;
                mask |= 1 << 4;
            } else if ((setting + i)->addr == 0x30) {
                dpll_predivp_sel = ((((setting + i)->value) & 0x04) >> 2) + 1;
                cnt_clk_sel = ((((setting + i)->value) & 0x6) >> 1) + 3;
                mpll_predivp_sel = (((setting + i)->value) & 0x1) + 1;
                mask |= 1 << 5;
            } else if ((setting + i)->addr == 0x42) {
                dpll_mc = ((setting + i)->value) & 0x7;
                mask |= 1 << 6;
            } else if ((setting + i)->addr == 0x2f) {
                mpll_mc = ((setting + i)->value) & 0x7;
                mask |= 1 << 7;
            } else if ((setting + i)->addr == 0x35) {
                mpll_postdiv_sel = (((setting + i)->value) & 0x3) + 1;
                mask |= 1 << 8;
            } else if ((setting + i)->addr == 0x43) {
                pclkdiv_sel = ((((setting + i)->value) & 0x6) >> 1) + 3;
                mask |= 1 << 9;
            }
        }

        if (mask == 0x3ff) break;
    }

    //default value
    if ((mask & 0x01) == 0) {
        clk_sel = (0x10 & 0x20) >> 5;
        timerclk_ctrl = (0x10 & 0x1) + 1;
        rowclk_ctrl = ((0x10 & 0x10) >> 4) + 1;
        mask |= 1;
    }
    if ((mask & (1 << 1)) == 0) {
        row_clk_sel = (0x14 >> 4) + 5;
        mask |= 1 << 1;
    }
    if ((mask & (1 << 2)) == 0) {
        dpll_nc_h = 0x00;
        mpll_nc_h = 0x00;
        mask |= 1 << 2;
    }
    if ((mask & (1 << 3)) == 0) {
        dpll_nc_l = 0x90;
        mask |= 1 << 3;
    }
    if ((mask & (1 << 4)) == 0) {
        mpll_nc_l = 0x78;
        mask |= 1 << 4;
    }
    if ((mask & (1 << 5)) == 0) {
        dpll_predivp_sel = ((0x02 & 0x04) >> 2) + 1;
        cnt_clk_sel = ((0x02 & 0x6) >> 1) + 3;
        mpll_predivp_sel = (0x02 & 0x1) + 1;
        mask |= 1 << 5;
    }
    if ((mask & (1 << 6)) == 0) {
        dpll_mc = 0x05 & 0x7;
        mask |= 1 << 6;
    }
    if ((mask & (1 << 7)) == 0) {
        mpll_mc = 0x05 & 0x7;
        mask |= 1 << 7;
    }
    if ((mask & (1 << 8)) == 0) {
        mpll_postdiv_sel = (0x09 & 0x3) + 1;
        mask |= 1 << 8;
    }
    if ((mask & (1 << 9)) == 0) {
        pclkdiv_sel = ((0x05 & 0x6) >> 1) + 3;
        mask |= 1 << 9;
    }

    if (mask != 0x3ff) {
        SNS_ERR("get setting sclk fail, mask:0x%x\n", mask);
        return -1;
    }

    dpll_mc = dpll_mc & 0x7;
    if (dpll_mc == 0) {
        dpll_mc = 1;
    } else if (dpll_mc == 1) {
        dpll_mc = 1.5;
    } else if (dpll_mc == 2) {
        dpll_mc = 2;
    } else if (dpll_mc == 3) {
        dpll_mc = 2.5;
    } else if (dpll_mc == 4) {
        dpll_mc = 3;
    } else if (dpll_mc == 5) {
        dpll_mc = 4;
    } else if (dpll_mc == 6) {
        dpll_mc = 6;
    } else if (dpll_mc == 7) {
        dpll_mc = 8;
    } else {
    }


    mpll_mc = mpll_mc & 0x7;
    if (mpll_mc == 0) {
        mpll_mc = 1;
    } else if (mpll_mc == 1) {
        mpll_mc = 1.5;
    } else if (mpll_mc == 2) {
        mpll_mc = 2;
    } else if (mpll_mc == 3) {
        mpll_mc = 2.5;
    } else if (mpll_mc == 4) {
        mpll_mc = 3;
    } else if (mpll_mc == 5) {
        mpll_mc = 4;
    } else if (mpll_mc == 6) {
        mpll_mc = 6;
    } else if (mpll_mc == 7) {
        mpll_mc = 8;
    } else {
    }

    dpll_nc = dpll_nc_h << 8 | dpll_nc_l;
    mpll_nc = mpll_nc_h << 8 | mpll_nc_l;

    if (clk_sel == 1) {
        *sclk = ((((((float)(inck * 1000 * 1000) * dpll_nc) / dpll_predivp_sel) / dpll_mc) / cnt_clk_sel) / row_clk_sel);
    } else {
        *sclk = ((((((((float)(inck * 1000 * 1000) * mpll_nc) / mpll_predivp_sel) / mpll_mc) / mpll_postdiv_sel) / pclkdiv_sel) / timerclk_ctrl) / rowclk_ctrl);
    }

    /*SNS_DBG("%s pre_div0=0x%x, pre_div=0x%x, multiplier=0x%x, post_div=0x%x, sram_div=0x%x, st_div=0x%x, t_div=0x%x \n", \
          __func__, pre_div0, pre_div, multiplier, post_div, sram_div, st_div, t_div); */

    return SNS_SUCCESS;
}

AX_S32 os04d10_select_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array **setting, AX_U32 *cnt)
{
    SNS_STATE_OBJ *sns_obj = AX_NULL;

    SNS_CHECK_VALUE_RANGE_VALID(nPipeId, 0, AX_VIN_MAX_PIPE_NUM - 1);
    SENSOR_GET_CTX(nPipeId, sns_obj);
    SNS_CHECK_PTR_VALID(sns_obj);

    switch (sns_obj->eImgMode) {
    /* 2 lane */
    case e_OS04D10_2lane_2560x1440_10bit_Linear_30fps:
        *setting = OS04D10_2lane_2560x1440_10bit_Linear_30fps;
        *cnt = sizeof(OS04D10_2lane_2560x1440_10bit_Linear_30fps) / sizeof(camera_i2c_reg_array);
        break;
    case e_OS04D10_2lane_1280x720_10bit_Linear_60fps:
        *setting = OS04D10_2lane_1280x720_10bit_Linear_60fps;
        *cnt = sizeof(OS04D10_2lane_1280x720_10bit_Linear_60fps) / sizeof(camera_i2c_reg_array);
        break;

    default:
        SNS_ERR("it's not supported. pipe=%d, setting mode=%d\n", nPipeId, sns_obj->eImgMode);
        return -1;
    }

    SNS_DBG("pipe=%d, setting mode=%d\n", nPipeId, sns_obj->eImgMode);

    return SNS_SUCCESS;
}

AX_S32 os04d10_write_settings(ISP_PIPE_ID nPipeId)
{
    AX_U32 i = 0;
    AX_S32 ret = 0;
    AX_U32 reg_cnt = 0;
    camera_i2c_reg_array *setting = AX_NULL;

    ret = os04d10_select_setting(nPipeId, &setting, &reg_cnt);
    if (ret) {
        return ret;
    }

    for (i = 0; i < reg_cnt; i++) {
        os04d10_write_register(nPipeId, (setting + i)->addr, ((setting + i)->value));
#ifdef SENSOR_DEBUG
        rt_thread_mdelay(2 * 1000);
        AX_U8 val = os04d10_reg_read(nPipeId, (default_setting + i)->addr);
        SNS_DBG(" addr: 0x%04x write:0x%02x read:0x%02x \r\n",
                (default_setting + i)->addr, (default_setting + i)->value, val);
#endif
    }

    return SNS_SUCCESS;
}
