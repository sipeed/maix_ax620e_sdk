/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OS04A10_REG_H__
#define __OS04A10_REG_H__

#include "ax_base_type.h"

#define OS04A10_SLAVE_ADDR1      (0x36)    /**< i2c slave address of the OS04a10 camera sensor */
#define OS04A10_SLAVE_ADDR2      (0x10)    /**< i2c slave address of the OS04a10 camera sensor */
#define OS04A10_SLAVE_ADDR      (0x36)    /**< i2c slave address of the OS04a10 camera sensor */
#define OS04A10_SENSOR_CHIP_ID  (0x530441)
#define OS04A10_ADDR_BYTE       (2)
#define OS04A10_DATA_BYTE       (1)
#define OS04A10_SWAP_BYTES      (1)
#define OS04A10_INCK_24M        (24)

/* Exposure control related registers */
#define     OS04A10_LONG_EXP_LINE_H     (0x3501)  /* bit[7:0], long frame exposure in unit of rows */
#define     OS04A10_LONG_EXP_LINE_L     (0x3502)  /* bit[7:0], long frame exposure in unit of rows */
#define     OS04A10_LONG_AGAIN_H        (0x3508)  /* bit[4:0], real gain[8:4] long frame */
#define     OS04A10_LONG_AGAIN_L        (0x3509)  /* bit[7:4], real gain[3:0] long frame */
#define     OS04A10_LONG_DGAIN_H        (0x350A)  /* bit[3:0], real gain[13:10] long frame */
#define     OS04A10_LONG_DGAIN_M        (0x350B)  /* bit[7:0], real gain[9:2] long frame */
#define     OS04A10_LONG_DGAIN_L        (0x350C)  /* bit[7:6], real gain[1:0] long frame */

#define     OS04A10_SHORT_EXP_LINE_H    (0x3541)  /* bit[7:0], short frame exposure in unit of rows */
#define     OS04A10_SHORT_EXP_LINE_L    (0x3542)  /* bit[7:0], short frame exposure in unit of rows */
#define     OS04A10_SHORT_AGAIN_H       (0x3548)  /* bit[4:0], real gain[8:4] short frame */
#define     OS04A10_SHORT_AGAIN_L       (0x3549)  /* bit[7:4], real gain[3:0] short frame */
#define     OS04A10_SHORT_DGAIN_H       (0x354A)  /* bit[3:0], real gain[13:10] short frame */
#define     OS04A10_SHORT_DGAIN_M       (0x354B)  /* bit[7:0], real gain[9:2] short frame */
#define     OS04A10_SHORT_DGAIN_L       (0x354C)  /* bit[7:6], real gain[1:0] short frame */

#define     OS04A10_VS_EXP_LINE_H       (0x3581)  /* bit[7:0], very short frame exposure in unit of rows */
#define     OS04A10_VS_EXP_LINE_L       (0x3582)  /* bit[7:0], very short frame exposure in unit of rows */
#define     OS04A10_VS_AGAIN_H          (0x3588)  /* bit[4:0], real gain[8:4] very short frame */
#define     OS04A10_VS_AGAIN_L          (0x3589)  /* bit[7:4], real gain[3:0] very short frame */
#define     OS04A10_VS_DGAIN_H          (0x358A)  /* bit[3:0], real gain[13:10] very short frame */
#define     OS04A10_VS_DGAIN_M          (0x358B)  /* bit[7:0], real gain[9:2] very short frame */
#define     OS04A10_VS_DGAIN_L          (0x358C)  /* bit[7:6], real gain[1:0] very short frame */

#define     OS04A10_VTS_H               (0x380E)  /* bit[7:0], vts[15:8] */
#define     OS04A10_VTS_L               (0x380F)  /* bit[7:0], vts[7:0] */

#define     OS04A10_GROUP_SWCTRL1       (0x320D)  /* */
#define     OS04A10_GROUP_ACCESS1       (0x3208)  /*  */
#define     OS04A10_HCG_LCG             (0x376C)  /*  */
#define     OS04A10_GROUP_ACCESS2       (0x3208)  /* */
#define     OS04A10_GROUP_SWCTRL2       (0x320D)  /*  */
#define     OS04A10_GROUP10_STAY_NUM    (0x320A)  /*  */
#define     OS04A10_SW_GROUP_ACCESS     (0x320E)  /*  */

/* VTS 4LINE */
#define OS04A10_VTS_10BIT_4M30_SDR             (0x0CB0)
#define OS04A10_VTS_10BIT_4M25_SDR             (0x0F39)
#define OS04A10_VTS_10BIT_4M15_SDR             (0x195f)

#define OS04A10_VTS_12BIT_4M30_SDR             (0x0658)
#define OS04A10_VTS_12BIT_4M25_SDR             (0x07A0)

#define OS04A10_VTS_10BIT_4M30_HDR_2X          (0x0658)
#define OS04A10_VTS_10BIT_4M25_HDR_2X          (0x07A0)

#define OS04A10_VTS_12BIT_4M60_SDR             (0x0650)
/* VTS 2LINE */
#define OS04A10_VTS_10BIT_4M30_SDR_2LINE       (0x0667)
#define OS04A10_VTS_10BIT_4M25_SDR_2LINE       (0x079C)

#define OS04A10_VTS_10BIT_4M30_HDR_2X_2LINE    (0x0658)
#define OS04A10_VTS_10BIT_4M25_HDR_2X_2LINE    (0x079C)


typedef enum {
    e_OS04A10_setting_sel_min = 0,
    /* 4 lane */
    e_OS04A10_4lane_2688x1520_10bit_Linear_30fps,
    e_OS04A10_4lane_2688x1520_12bit_Linear_30fps,
    e_OS04A10_4lane_2688x1520_10bit_2Stagger_HDR_30fps,
    e_OS04A10_4lane_2688x1520_10bit_Linear_60fps,

    e_OS04A10_setting_special_idx = 0x20, /* index = 32 */

    /* 2 lane */
    e_OS04A10_2lane_2688x1520_10bit_Linear_30fps,
    e_OS04A10_2lane_2688x1520_10bit_2Stagger_HDR_30fps,
    e_OS04A10_2lane_2688x1520_10bit_Linear_60fps,

    e_OS04A10_setting_sel_max
} OS04A10_SETTING_SEL_E;

AX_S32 os04a10_sensor_i2c_init(ISP_PIPE_ID nPipeId);
AX_S32 os04a10_sensor_i2c_exit(ISP_PIPE_ID nPipeId);

AX_S32 os04a10_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio, AX_U8 nValue);

AX_S32 os04a10_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData);
AX_S32 os04a10_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr);
AX_S32 os04a10_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data);

AX_U32 os04a10_get_hts(ISP_PIPE_ID nPipeId);
AX_F32 os04a10_get_sclk(ISP_PIPE_ID nPipeId);
AX_U32 os04a10_get_vts(ISP_PIPE_ID nPipeId);
AX_U32 os04a10_get_vs_hts(ISP_PIPE_ID nPipeId);

AX_S32 os04a10_write_settings(ISP_PIPE_ID nPipeId, AX_U32 setindex);

AX_S32 os04a10_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo);
AX_S32 os04a10_get_bus_num(ISP_PIPE_ID nPipeId);

AX_S32 os04a10_set_slaveaddr(ISP_PIPE_ID nPipeId, AX_U8 nslaveaddr);

#endif  //end __OS04A10_REG_H__
