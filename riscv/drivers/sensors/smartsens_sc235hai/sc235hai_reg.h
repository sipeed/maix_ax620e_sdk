/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SC235HAI_REG_H__
#define __SC235HAI_REG_H__

#include "ax_base_type.h"

#define SC235HAI_SLAVE_ADDR1             (0x30)    /**< i2c slave address of the SC235HAI camera sensor sid0: 0*/
#define SC235HAI_SLAVE_ADDR2             (0x32)    /**< i2c slave address of the SC235HAI camera sensor sid0: 1*/
#define SC235HAI_SENSOR_ID               (0xcb6a)
#define SC235HAI_ADDR_BYTE               (2)
#define SC235HAI_DATA_BYTE               (1)
#define SC235HAI_SWAP_BYTES              (1)

#define SC235HAI_SENSOR_ID_REG_H         (0x3107)
#define SC235HAI_SENSOR_ID_REG_L         (0x3108)

/* Exposure control related registers */
#define     SC235HAI_LONG_EXP_LINE_H     (0x3e00)  /* bit[3:0], long frame exposure in unit of rows */
#define     SC235HAI_LONG_EXP_LINE_M     (0x3e01)  /* bit[7:0], long frame exposure in unit of rows */
#define     SC235HAI_LONG_EXP_LINE_L     (0x3e02)  /* bit[7:4], long frame exposure in unit of rows */

#define     SC235HAI_SHORT_EXP_LINE_H    (0x3e22)  /* bit[4:0], short frame exposure in unit of rows */
#define     SC235HAI_SHORT_EXP_LINE_M    (0x3e04)  /* bit[7:0], short frame exposure in unit of rows */
#define     SC235HAI_SHORT_EXP_LINE_L    (0x3e05)  /* bit[7:4], short frame exposure in unit of rows */

#define     SC235HAI_LONG_AGAIN_H        (0x3e08)
#define     SC235HAI_LONG_AGAIN_L        (0x3e09)
#define     SC235HAI_LONG_DGAIN_H        (0x3e06)
#define     SC235HAI_LONG_DGAIN_L        (0x3e07)

#define     SC235HAI_SHORT_AGAIN_H       (0x3e12)
#define     SC235HAI_SHORT_AGAIN_L       (0x3e13)
#define     SC235HAI_SHORT_DGAIN_H       (0x3e10)
#define     SC235HAI_SHORT_DGAIN_L       (0x3e11)

#define     SC235HAI_VTS_L_H             (0x320E)  /* bit[6:0], vts[15:8] */
#define     SC235HAI_VTS_L_L             (0x320F)  /* bit[7:0], vts[7:0] */
#define     SC235HAI_VTS_S_H             (0x3E23)  /* bit[6:0], vts[15:8] */
#define     SC235HAI_VTS_S_L             (0x3E24)  /* bit[7:0], vts[7:0] */

#define     SC235HAI_HTS_L_H             (0x320C)
#define     SC235HAI_HTS_L_L             (0x320D)

AX_S32 sc235hai_sensor_i2c_init(ISP_PIPE_ID nPipeId);
AX_S32 sc235hai_sensor_i2c_exit(ISP_PIPE_ID nPipeId);
AX_S32 sc235hai_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData);
AX_S32 sc235hai_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr);
AX_S32 sc235hai_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data);

AX_S32 sc235hai_hw_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio, AX_U8 nValue);
AX_U32 sc235hai_get_hts(ISP_PIPE_ID nPipeId);
AX_U32 sc235hai_get_vts(ISP_PIPE_ID nPipeId);
AX_U32 sc235hai_set_vts(ISP_PIPE_ID nPipeId, AX_U32 vts);
AX_U32 sc235hai_set_vts_s(ISP_PIPE_ID nPipeId, AX_U32 vts);

AX_S32 sc235hai_get_vts_from_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array *setting, AX_U32 reg_cnt, AX_U32 *vts);

AX_S32 sc235hai_select_setting(ISP_PIPE_ID nPipeId, camera_i2c_reg_array **setting, AX_U32 *cnt);
AX_S32 sc235hai_write_settings(ISP_PIPE_ID nPipeId);

AX_S32 sc235hai_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo);
AX_S32 sc235hai_get_bus_num(ISP_PIPE_ID nPipeId);

AX_S32 sc235hai_set_slaveaddr(ISP_PIPE_ID nPipeId, AX_U8 nslaveaddr);

AX_U32 sc235hai_get_vs_vts(ISP_PIPE_ID nPipeId);

AX_F32 sc235hai_get_exp_offset(ISP_PIPE_ID nPipeId);

//AX_S32 sc235hai_get_sensor_stream_ctrl(ISP_PIPE_ID nPipeId);


typedef enum {
    e_SC235HAI_setting_sel_min = 0,
    e_SC235HAI_MIPI_24M_396MBPS_2LANE_1920x1080_10BIT_SDR_30FPS,
    e_SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS,

    e_SC235HAI_setting_special_idx = 0x20, /* index = 32 */

    e_SC235HAI_MIPI_27M_396MBPS_2LANE_1920x1080_10BIT_SDR_30FPS,
    e_SC235HAI_MIPI_27M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS,
    e_SC235HAI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS,

    e_SC235HAI_setting_sel_max
} SC235HAI_SETTING_SEL_E;

#define AX_SNS_ERR_NOMEM        SNS_ERR_CODE_NOT_MEM
#define AX_SNS_SUCCESS          SNS_SUCCESS
#define AX_SNS_ERR_NOT_INIT     SNS_ERR_CODE_INIT_FAILD
#define AX_SNS_ERR_NOT_SUPPORT  SNS_ERR_CODE_ILLEGAL_PARAMS

#define AX_SNS_ERR_NULL_PTR  -1
#define AX_SNS_ERR_NOT_MATCH -1
#define AX_SNS_ERR_BAD_ADDR      SNS_ERR_CODE_ILLEGAL_PARAMS
#define AX_SNS_ERR_ILLEGAL_PARAM SNS_ERR_CODE_ILLEGAL_PARAMS
#define SNS_INFO SNS_DBG
#endif  //end __SC235HAI_REG_H__
