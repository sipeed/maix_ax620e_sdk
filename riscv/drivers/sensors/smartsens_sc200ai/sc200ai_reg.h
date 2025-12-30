/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __SC200AI_REG_H__
#define __SC200AI_REG_H__

#include "ax_base_type.h"

#define SC200AI_SLAVE_ADDR1         (0x30)    /**< i2c slave address of the SC200AI camera sensor sid0: 0*/
#define SC200AI_SLAVE_ADDR2         (0x32)    /**< i2c slave address of the SC200AI camera sensor sid0: 1*/
#define SC200AI_SENSOR_CHIP_ID      (0xcb1c)
#define SC200AI_ADDR_BYTE           (2)
#define SC200AI_DATA_BYTE           (1)
#define SC200AI_SWAP_BYTES          (1)

/* Exposure control related registers */
#define SC200AI_EXPLINE_LONG_H      (0x3e00)  /* bit[3:0], long frame exposure in unit of rows */
#define SC200AI_EXPLINE_LONG_M      (0x3e01)  /* bit[7:0], long frame exposure in unit of rows */
#define SC200AI_EXPLINE_LONG_L      (0x3e02)  /* bit[7:4], long frame exposure in unit of rows */

#define SC200AI_EXPLINE_SHORT_H     (0x3e22)  /* bit[3:0], short frame exposure in unit of rows */
#define SC200AI_EXPLINE_SHORT_M     (0x3e04)  /* bit[7:0], short frame exposure in unit of rows */
#define SC200AI_EXPLINE_SHORT_L     (0x3e05)  /* bit[7:4], short frame exposure in unit of rows */

#define SC200AI_AGAIN_LONG_H        (0x3e08)
#define SC200AI_AGAIN_LONG_L        (0x3e09)
#define SC200AI_AGAIN_SHORT_H       (0x3e12)
#define SC200AI_AGAIN_SHORT_L       (0x3e13)

#define SC200AI_DGAIN_LONG_H        (0x3e06)
#define SC200AI_DGAIN_LONG_L        (0x3e07)
#define SC200AI_DGAIN_SHORT_H       (0x3e10)
#define SC200AI_DGAIN_SHORT_L       (0x3e11)

#define SC200AI_VTS_LONG_H          (0x320E)  /* bit[6:0], vts[15:8] */
#define SC200AI_VTS_LONG_L          (0x320F)  /* bit[7:0], vts[7:0] */
#define SC200AI_VTS_SHORT_H         (0x3E23)  /* bit[6:0], vts[15:8] */
#define SC200AI_VTS_SHORT_L         (0x3E24)  /* bit[7:0], vts[7:0] */

#define SC200AI_HTS_LONG_H          (0x320C)
#define SC200AI_HTS_LONG_L          (0x320D)

typedef enum {
    e_SC200AI_setting_sel_min = 0,
    e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS,
    e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS,

    /* sdr master/slave */
    e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_MASTER,
    e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_25FPS_SYNC_SLAVE,
    e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_MASTER,
    e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1080_10BIT_SDR_30FPS_SYNC_SLAVE,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_SDR_60FPS_SYNC_SLAVE,

    /* hdr master/slave */
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_MASTER,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_25FPS_SYNC_SLAVE,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_MASTER,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1080_10BIT_HDR_30FPS_SYNC_SLAVE,

    e_SC200AI_MIPI_24M_742P5MBPS_1LANE_1920x1080_10BIT_SDR_25FPS,
    e_SC200AI_MIPI_24M_742P5MBPS_1LANE_1920x1080_10BIT_SDR_30FPS,

    e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_25FPS,
    e_SC200AI_MIPI_24M_371P25MBPS_2LANE_1920x1088_10BIT_SDR_30FPS,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_25FPS,
    e_SC200AI_MIPI_24M_792MBPS_2LANE_1920x1088_10BIT_HDR_30FPS,

    e_SC200AI_setting_sel_max
} SC200AI_SETTING_SEL_E;

AX_S32 sc200ai_reset(ISP_PIPE_ID nPipeId, AX_U32 nResetGpio, AX_U8 nValue);
AX_S32 sc200ai_sensor_i2c_init(ISP_PIPE_ID nPipeId);
AX_S32 sc200ai_sensor_i2c_exit(ISP_PIPE_ID nPipeId);
AX_S32 sc200ai_read_register(ISP_PIPE_ID nPipeId, AX_U32 nAddr, AX_U32 *pData);
AX_S32 sc200ai_reg_read(ISP_PIPE_ID nPipeId, AX_U32 addr);
AX_S32 sc200ai_write_register(ISP_PIPE_ID nPipeId, AX_U32 addr, AX_U32 data);

AX_U32 sc200ai_get_vts(ISP_PIPE_ID nPipeId);
AX_U32 sc200ai_set_vts(ISP_PIPE_ID nPipeId, AX_U32 vts);
AX_U32 sc200ai_set_vs_vts(ISP_PIPE_ID nPipeId, AX_U32 vts);

AX_U32 sc200ai_get_vts_s(ISP_PIPE_ID nPipeId);
AX_U32 sc200ai_set_vts_s(ISP_PIPE_ID nPipeId, AX_U32 vts);

AX_S32 sc200ai_write_settings(ISP_PIPE_ID nPipeId, AX_U32 setindex);

AX_S32 sc200ai_set_bus_info(ISP_PIPE_ID nPipeId, AX_SNS_COMMBUS_T tSnsBusInfo);
AX_S32 sc200ai_get_bus_num(ISP_PIPE_ID nPipeId);

AX_S32 sc200ai_set_slaveaddr(ISP_PIPE_ID nPipeId, AX_U8 nslaveaddr);

#endif  //end __SC200AI_REG_H__
